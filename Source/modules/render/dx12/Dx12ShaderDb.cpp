#include <Config.h>

#if ENABLE_DX12

#include <string>
#include <coalpy.core/Assert.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.core/String.h>
#include <coalpy.files/FileWatcher.h>

#include "Dx12ShaderDb.h" 
#include "Dx12Compiler.h"
#include "Dx12Device.h"
#include <coalpy.core/ByteBuffer.h>

#include <iostream>
#include <windows.h>
#include <d3d12.h>
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
    std::set<Dx12FileLookup> files;
    bool success;
};

Dx12ShaderDb::Dx12ShaderDb(const ShaderDbDesc& desc)
: m_compiler(desc)
, m_desc(desc)
{
    if (m_desc.enableLiveEditing)
        startLiveEdit();
}

Dx12ShaderDb::~Dx12ShaderDb()
{
    if (m_desc.enableLiveEditing)
        stopLiveEdit();

    int unresolvedShaders = 0;
    m_shaders.forEach([&unresolvedShaders, this](ShaderHandle handle, ShaderState* state)
    {
        if (m_desc.resolveOnDestruction)
            resolve(handle);
        else
            unresolvedShaders += state->compileState == nullptr ? 0 : 1;

        if (state->shaderBlob)
            state->shaderBlob->Release();

        if (state->csPso)
        {
            ID3D12PipelineState* oldPso = state->csPso;
            oldPso->Release();
        }
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

    std::string inputPath = desc.path;
    std::string resolvedPath;
    FileUtils::getAbsolutePath(inputPath, resolvedPath);

    compileState->compileArgs = {};
    compileState->compileArgs.type = desc.type;
    compileState->shaderName = desc.name;
    compileState->mainFn = desc.mainFn;
    compileState->success = false;
    prepareIoJob(*compileState, resolvedPath);
    prepareCompileJobs(*compileState);

    ShaderHandle shaderHandle;
    ShaderState& shaderState = createShaderState(shaderHandle);
    shaderState.debugName = desc.name;
    shaderState.recipe.type = desc.type;
    shaderState.recipe.name = desc.name;
    shaderState.recipe.mainFn = desc.mainFn;
    FileUtils::getAbsolutePath(compileState->filePath, shaderState.recipe.path);
    shaderState.compileState = compileState;

    if (m_desc.enableLiveEditing)
    {
        Dx12FileLookup fileLookup(resolvedPath);
        std::unique_lock lock(m_dependencyMutex);
        m_fileToShaders[fileLookup].insert(shaderHandle);
        m_shadersToFiles[shaderHandle].insert(fileLookup);
    }

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
    compileState->success = false;

    prepareCompileJobs(*compileState);

    ShaderHandle shaderHandle;
    ShaderState& shaderState = createShaderState(shaderHandle);
    shaderState.recipe.type = desc.type;
    shaderState.recipe.name = desc.name;
    shaderState.recipe.mainFn = desc.mainFn;
    shaderState.recipe.source = desc.immCode;

    shaderState.debugName = desc.name;
    shaderState.compileState = compileState;
    compileState->shaderHandle = shaderHandle;
    m_desc.ts->execute(compileState->compileStep);
    return shaderHandle;
}

void Dx12ShaderDb::requestRecompile(ShaderHandle handle)
{
    std::shared_lock lock(m_shadersMutex);
    ShaderState* shaderState = nullptr;
    {
        bool containsShader = m_shaders.contains(handle);
        CPY_ASSERT(containsShader);
        if (!containsShader)
            return;

        shaderState = m_shaders[handle];
        CPY_ASSERT(shaderState != nullptr);
        if (!shaderState)
            return;

        if (shaderState->compileState)
            return;

        if (shaderState->compiling)
            return;
    }

    auto& recipe = shaderState->recipe;
    auto& compileState = *(new Dx12CompileState());
    compileState.shaderName = recipe.name;
    compileState.mainFn = recipe.mainFn;
    compileState.compileArgs.type = recipe.type;
    compileState.compileArgs.shaderName = recipe.name.c_str();
    compileState.compileArgs.mainFn = recipe.mainFn.c_str();
    compileState.mainFn = recipe.mainFn;
    prepareCompileJobs(compileState);

    if (!shaderState->recipe.source.empty())
    {
        compileState.compileArgs.source = (const char*)recipe.source.data();
        compileState.compileArgs.sourceSize = (int)recipe.source.size();
    }
    else
    {
        prepareIoJob(compileState, recipe.path);
        m_desc.ts->depends(compileState.compileStep, m_desc.fs->asTask(compileState.readStep));
    }

    compileState.shaderHandle = handle;
    Task patchTask = m_desc.ts->createTask(TaskDesc(
        [this, &compileState, shaderState](TaskContext& ctx)
        {
            std::unique_lock lock(m_shadersMutex);
            shaderState->compiling = true;
            shaderState->compileState = &compileState;
        }));

    m_desc.ts->depends(patchTask, compileState.compileStep);
    compileState.compileStep = patchTask;
    m_desc.ts->execute(compileState.compileStep);
}

void Dx12ShaderDb::prepareIoJob(Dx12CompileState& compileState, const std::string& resolvedPath)
{
    compileState.filePath = resolvedPath;
    compileState.compileArgs.shaderName = compileState.shaderName.c_str();
    compileState.compileArgs.debugName = compileState.filePath.c_str();
    compileState.compileArgs.mainFn = compileState.mainFn.c_str();

    const auto& readPath = compileState.filePath;
    compileState.readStep = m_desc.fs->read(FileReadRequest(readPath, [&compileState, this](FileReadResponse& response){
        if (response.status == FileStatus::Reading)
        {
            compileState.buffer.append((const u8*)response.buffer, response.size);
        }
        else if (response.status == FileStatus::Success)
        {
            compileState.compileArgs.source = (const char*)compileState.buffer.data();
            compileState.compileArgs.sourceSize = (int)compileState.buffer.size();
        }
        else if (response.status == FileStatus::Fail)
        {
            compileState.compileArgs.source = nullptr;
            compileState.compileArgs.sourceSize = 0u;
            if (m_desc.onErrorFn)
            {
                std::stringstream ss;
                ss << "Failed reading " << compileState.filePath.c_str() << ". Reason: " << IoError2String(response.error);
                m_desc.onErrorFn(compileState.shaderHandle, compileState.shaderName.c_str(), ss.str().c_str());
            }
        }
    }));

    if (m_desc.enableLiveEditing)
    {
        Dx12FileLookup fileLookup(resolvedPath);
        compileState.files.insert(fileLookup);
    }
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

        if (result && m_desc.enableLiveEditing)
        {
            Dx12FileLookup fileLookup(path);
            compileState.files.insert(fileLookup);
        }

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
            compileState.success = success;
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

        if (m_desc.enableLiveEditing)
        {
            std::shared_lock lock(m_dependencyMutex);
            if (compileState->success)
            {
                //step 1, clear the dependencies
                {
                    auto& setToClear = m_shadersToFiles[handle];
                    for (auto fileInSet : setToClear)
                    {
                        m_fileToShaders[fileInSet].erase(handle);
                    }

                    m_shadersToFiles.erase(handle);
                }

                //step 2, flush new dependencies
                for (auto& fileUsed : compileState->files)
                {
                    m_fileToShaders[fileUsed].insert(handle);
                }
                m_shadersToFiles[handle].insert(compileState->files.begin(), compileState->files.end());
            }
        }

        if (compileState->readStep.valid())
            m_desc.fs->closeHandle(compileState->readStep);
        m_desc.ts->cleanTaskTree(compileState->compileStep);

        {
            std::shared_lock lock(m_shadersMutex);
            delete compileState;
            
            if (m_parentDevice != nullptr && shaderState->recipe.type == ShaderType::Compute)
            {
                bool psoResult = updateComputePipelineState(*shaderState);
                if (!psoResult && m_desc.onErrorFn != nullptr)
                    m_desc.onErrorFn(handle, shaderState->debugName.c_str(), "Compute PSO creation failed. Check D3D12 error log.");
            }

            shaderState->compiling = false;
        }
    }
}

bool Dx12ShaderDb::updateComputePipelineState(ShaderState& state)
{
    CPY_ASSERT(m_parentDevice != nullptr);
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = &m_parentDevice->defaultComputeRootSignature();
    desc.CS.pShaderBytecode = state.shaderBlob->GetBufferPointer();
    desc.CS.BytecodeLength = state.shaderBlob->GetBufferSize();
    ID3D12PipelineState* pso;
    HRESULT result = m_parentDevice->device().CreateComputePipelineState(&desc, DX_RET(pso));
    ID3D12PipelineState* oldPso = state.csPso;
    state.csPso = pso;
    if (oldPso)
        oldPso->Release();
    return result == S_OK;
}

bool Dx12ShaderDb::isValid(ShaderHandle handle) const
{
    std::shared_lock lock(m_shadersMutex);
    const auto& state = m_shaders[handle];
    return state->ready && state->success;
}

void Dx12ShaderDb::startLiveEdit()
{
    auto liveEditFn = [this](const std::set<std::string>& filesChanged)
    {
        std::set<ShaderHandle> handlesToRecompile;
        for (const auto& fileChanged : filesChanged)
        {
            std::string resolvedFileName;
            FileUtils::getAbsolutePath(fileChanged, resolvedFileName);
            
            {
                std::unique_lock lock(m_dependencyMutex);
                FileToShaderHandlesMap::iterator shadersIt = m_fileToShaders.find(resolvedFileName);
                if (shadersIt == m_fileToShaders.end())
                    continue;

                handlesToRecompile.insert(shadersIt->second.begin(), shadersIt->second.end());
            }
        }

        for (auto& handle : handlesToRecompile)
        {

            //request recompile.
            requestRecompile(handle);
        }
    };

    m_liveEditWatcher.start(
        ".",
        liveEditFn
    );
}

void Dx12ShaderDb::stopLiveEdit()
{
    m_liveEditWatcher.stop();
}

Dx12FileLookup::Dx12FileLookup()
: filename(""), hash(0u)
{
}

Dx12FileLookup::Dx12FileLookup(const char* file)
: filename(file)
{
    hash = stringHash(filename);
}

Dx12FileLookup::Dx12FileLookup(const std::string& filename)
: filename(filename)
{
    hash = stringHash(filename);
}

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
    return new Dx12ShaderDb(desc);
}

}

#endif
