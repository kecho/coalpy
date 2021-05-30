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

namespace coalpy
{

struct Dx12CompileState
{
    ByteBuffer buffer;
    std::string shaderName;
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
}

ShaderHandle Dx12ShaderDb::requestCompile(const ShaderDesc& desc)
{
    auto* compileState = new Dx12CompileState;

    compileState->compileArgs = {};
    compileState->shaderName = desc.name;
    compileState->mainFn = desc.mainFn;
    compileState->compileArgs.type = desc.type;
    compileState->compileArgs.shaderName = compileState->shaderName.c_str();
    compileState->compileArgs.mainFn = compileState->mainFn.c_str();

    std::string readPath = desc.path;
    compileState->readStep = m_desc.fs->read(FileReadRequest(readPath, [compileState, this](FileReadResponse& response){
        if (response.status == FileStatus::Reading)
        {
            compileState->buffer.append(response.buffer, response.size);
        }
        else if (response.status == FileStatus::ReadingSuccessEof)
        {
            compileState->compileArgs.source = (const char*)compileState->buffer.data();
        }
        else if (response.status == FileStatus::ReadingFail)
        {
            std::cout << "Failed reading " << compileState->shaderName;
        }
    }));

    compileState->compileStep = m_desc.ts->createTask(TaskDesc(
        desc.name,
        [compileState, this](TaskContext& ctx)
    {
        m_compiler.compileShader(compileState->compileArgs);
    }));

    compileState->compileArgs.onInclude = [compileState, this](const char* path, ByteBuffer& buffer)
    {
        std::string strpath = path;
        bool result = false;
        auto handle = m_desc.fs->read(FileReadRequest(strpath,
        [&buffer, &result](FileReadResponse& response){
            if (response.status == FileStatus::Reading)
            {
                buffer.append(response.buffer, response.size);    
            }
            else if (response.status == FileStatus::ReadingSuccessEof)
            {
                result = true;
            }
        }));

        m_desc.fs->wait(handle);
        m_desc.fs->closeHandle(handle);

        return result;
    };

    compileState->compileArgs.onFinished = [compileState, this](bool success, IDxcBlob* resultBlob)
    {
        if (success)
        {
            std::cout << "Success compiling " << compileState->shaderName;
        }
        else
        {
            std::cout << "Failed compiling " << compileState->shaderName;
        }

        {
            std::unique_lock lock(m_shadersMutex);
            auto& shaderState = m_shaders[compileState->shaderHandle];
            shaderState.ready = true;
            shaderState.success = success;
        }
    };

    ShaderHandle shaderHandle;
    {
        std::unique_lock lock(m_shadersMutex);
        auto& state = m_shaders.allocate(shaderHandle);
        state.ready = false;
        state.success = false;
        state.compileState = compileState;
    };

    compileState->shaderHandle = shaderHandle;
    m_desc.ts->depends(compileState->compileStep, m_desc.fs->asTask(compileState->readStep));
    m_desc.ts->execute(compileState->compileStep);
    return shaderHandle;
}

void Dx12ShaderDb::resolve(ShaderHandle handle)
{
    Dx12CompileState* compileState = nullptr;
    {
        std::shared_lock lock(m_shadersMutex);
        compileState = m_shaders[handle].compileState;
    }

    if (compileState == nullptr)
        return;

    m_desc.ts->wait(compileState->compileStep);
    if (compileState->readStep.valid())
        m_desc.fs->closeHandle(compileState->readStep);
    m_desc.ts->cleanTaskTree(compileState->compileStep);
    
    {
        std::unique_lock lock(m_shadersMutex);
        auto& state = m_shaders[handle];
        delete state.compileState;
        state.compileState = nullptr;
        compileState = nullptr;
    }
}

bool Dx12ShaderDb::isValid(ShaderHandle handle) const
{
    std::shared_lock lock(m_shadersMutex);
    const auto& state = m_shaders[handle];
    return state.ready && state.success;
}

ShaderHandle Dx12ShaderDb::requestCompile(const ShaderInlineDesc& desc)
{
    return ShaderHandle();
}

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
    return new Dx12ShaderDb(desc);
}

}

#endif
