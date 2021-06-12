#include <Config.h>

#if ENABLE_DX12

#include <string>
#include <coalpy.core/Assert.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.tasks/ITaskSystem.h>

#include "Dx12ShaderDb.h" 
#include "Dx12Compiler.h"
#include <coalpy.core/ByteBuffer.h>

#include <iostream>
#include <windows.h>
#include <dxc/inc/dxcapi.h>

namespace coalpy
{

struct Dx12CompileState
{
    ByteBuffer buffer;
    std::string shaderName;
    std::string filePath;
    std::string mainFn;
    ShaderHandle shaderHandle;
    Dx12CompileArgs compileArgs;
    AsyncFileHandle readStep;
    Task compileStep;
};

Dx12ShaderDb::Dx12ShaderDb(const ShaderDbDesc& desc)
: m_compiler(desc)
, m_desc(desc)
{
}

Dx12ShaderDb::~Dx12ShaderDb()
{
    int unresolvedShaders = 0;
    m_shaders.forEach([&unresolvedShaders, this](ShaderHandle handle, ShaderState* state)
    {
        if (m_desc.resolveOnDestruction)
            resolve(handle);
        else
            unresolvedShaders += state->compileState == nullptr ? 0 : 1;

        if (state->shaderBlob)
            state->shaderBlob->Release();
        delete state;
    });

    CPY_ASSERT_FMT(unresolvedShaders == 0, "%d unresolved shaders. Expect memory leaks.", unresolvedShaders);
}

Dx12ShaderDb::ShaderState& Dx12ShaderDb::createShaderState(ShaderHandle& outHandle)
{
    ShaderState& shaderState = *(new ShaderState);
    {
        std::unique_lock lock(m_shadersMutex);
        auto& statePtr = m_shaders.allocate(outHandle);
        statePtr = &shaderState;
    }

    shaderState.ready = false;
    shaderState.success = false;
    shaderState.compiling = true;
    shaderState.shaderBlob = nullptr;
    return shaderState;
}

ShaderHandle Dx12ShaderDb::requestCompile(const ShaderDesc& desc)
{
    auto* compileState = new Dx12CompileState;

    compileState->compileArgs = {};
    compileState->shaderName = desc.name;
    compileState->mainFn = desc.mainFn;
    compileState->filePath = desc.path;
    compileState->compileArgs.type = desc.type;
    compileState->compileArgs.shaderName = compileState->shaderName.c_str();
    compileState->compileArgs.mainFn = compileState->mainFn.c_str();

    const auto& readPath = compileState->filePath;
    compileState->readStep = m_desc.fs->read(FileReadRequest(readPath, [compileState, this](FileReadResponse& response){
        if (response.status == FileStatus::Reading)
        {
            compileState->buffer.append((const u8*)response.buffer, response.size);
        }
        else if (response.status == FileStatus::Success)
        {
            compileState->compileArgs.source = (const char*)compileState->buffer.data();
            compileState->compileArgs.sourceSize = (int)compileState->buffer.size();
        }
        else if (response.status == FileStatus::Fail)
        {
            compileState->compileArgs.source = nullptr;
            compileState->compileArgs.sourceSize = 0u;
            if (m_desc.onErrorFn)
            {
                std::stringstream ss;
                ss << "Failed reading " << compileState->filePath.c_str() << ". Reason: " << IoError2String(response.error);
                m_desc.onErrorFn(compileState->shaderHandle, compileState->shaderName.c_str(), ss.str().c_str());
            }
        }
    }));

    prepareCompileJobs(*compileState);

    ShaderHandle shaderHandle;
    ShaderState& shaderState = createShaderState(shaderHandle);
    shaderState.compileState = compileState;
    
    compileState->shaderHandle = shaderHandle;
    m_desc.ts->depends(compileState->compileStep, m_desc.fs->asTask(compileState->readStep));
    m_desc.ts->execute(compileState->compileStep);
    return shaderHandle;
}

ShaderHandle Dx12ShaderDb::requestCompile(const ShaderInlineDesc& desc)
{
    auto* compileState = new Dx12CompileState;

    compileState->compileArgs = {};
    compileState->shaderName = desc.name;
    compileState->buffer.append((u8*)desc.immCode, strlen(desc.immCode) + 1);
    compileState->mainFn = desc.mainFn;
    compileState->compileArgs.type = desc.type;
    compileState->compileArgs.shaderName = compileState->shaderName.c_str();
    compileState->compileArgs.mainFn = compileState->mainFn.c_str();
    compileState->compileArgs.source = (const char*)compileState->buffer.data();
    compileState->compileArgs.sourceSize = (int)compileState->buffer.size();

    prepareCompileJobs(*compileState);

    ShaderHandle shaderHandle;
    ShaderState& shaderState = createShaderState(shaderHandle);
    shaderState.compileState = compileState;
    compileState->shaderHandle = shaderHandle;
    m_desc.ts->execute(compileState->compileStep);
    return shaderHandle;
}

void Dx12ShaderDb::prepareCompileJobs(Dx12CompileState& compileState)
{
    compileState.compileStep = m_desc.ts->createTask(TaskDesc(
        compileState.shaderName.c_str(),
        [&compileState, this](TaskContext& ctx)
    {
        m_compiler.compileShader(compileState.compileArgs);
    }));

    if (m_desc.onErrorFn)
        compileState.compileArgs.onError = [&compileState, this](const char* name, const char* errorString)
        {
            m_desc.onErrorFn(compileState.shaderHandle, name, errorString);
        };

    compileState.compileArgs.onInclude = [&compileState, this](const char* path, ByteBuffer& buffer)
    {
        std::string strpath = path;
        bool result = false;
        auto handle = m_desc.fs->read(FileReadRequest(strpath,
        [&buffer, &result](FileReadResponse& response){
            if (response.status == FileStatus::Reading)
            {
                buffer.append((const u8*)response.buffer, response.size);    
            }
            else if (response.status == FileStatus::Success)
            {
                result = true;
            }
        }));

        m_desc.fs->execute(handle);
        m_desc.fs->wait(handle);
        m_desc.fs->closeHandle(handle);
        return result;
    };

    compileState.compileArgs.onFinished = [&compileState, this](bool success, IDxcBlob* resultBlob)
    {
        {
            std::unique_lock lock(m_shadersMutex);
            auto& shaderState = m_shaders[compileState.shaderHandle];
            if (success && resultBlob)
            {
                resultBlob->AddRef();
                shaderState->shaderBlob = resultBlob;
            }
            shaderState->ready = true;
            shaderState->success = success;
        }
    };
}

void Dx12ShaderDb::resolve(ShaderHandle handle)
{
    CPY_ASSERT(handle.valid());

    if (!handle.valid())
        return;

    ShaderState* shaderState = nullptr;
    {
        std::shared_lock lock(m_shadersMutex);
        shaderState = m_shaders[handle];
    }

    Dx12CompileState* compileState = nullptr;
    while (shaderState->compiling)
    {
        {
            std::shared_lock lock(m_shadersMutex);
            compileState = shaderState->compileState;
            shaderState->compileState = nullptr;
        }

        if (compileState == nullptr)
        {
            m_desc.ts->yield();
            continue;
        }

        m_desc.ts->wait(compileState->compileStep);
        if (compileState->readStep.valid())
            m_desc.fs->closeHandle(compileState->readStep);
        m_desc.ts->cleanTaskTree(compileState->compileStep);
        delete compileState;
        shaderState->compiling = false;
    }
}

bool Dx12ShaderDb::isValid(ShaderHandle handle) const
{
    std::shared_lock lock(m_shadersMutex);
    const auto& state = m_shaders[handle];
    return state->ready && state->success;
}

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
    return new Dx12ShaderDb(desc);
}

}

#endif
