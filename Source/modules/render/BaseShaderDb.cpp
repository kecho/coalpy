#include <Config.h>

#include <string>
#include <sstream>
#include <coalpy.core/Assert.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.files/Utils.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/String.h>
#include <mutex>

#include "BaseShaderDb.h" 
#include "SpirvReflectionData.h"
#include <coalpy.core/ByteBuffer.h>

#include <iostream>
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#elif defined(__APPLE__)
#include "metal/MSLData.h"
#endif
#include <dxcapi.h>

namespace coalpy
{

const char* s_pdbDir = ".shader_pdb";

struct CompileState
{
    ByteBuffer buffer;
    std::string shaderName;
    std::string filePath;
    std::string mainFn;
    ShaderHandle shaderHandle;
    DxcCompileArgs compileArgs;
    AsyncFileHandle readStep;
    Task compileStep;
    std::set<FileLookup> files;
    bool success;
};

BaseShaderDb::BaseShaderDb(const ShaderDbDesc& desc)
: m_compiler(desc)
, m_desc(desc)
, m_liveEditWatcher(nullptr)
{
    if (m_desc.enableLiveEditing)
    {
        m_liveEditWatcher = desc.fw;
        CPY_ASSERT_MSG(m_liveEditWatcher != nullptr, "File watcher must not be null when live editing is turned on.");
        startLiveEdit();
    }
}

void BaseShaderDb::preparePdbDir()
{
    if (m_createdPdbDir || !m_desc.dumpPDBs)
        return;
    
#ifdef __linux__
    std::cout << "Shader PDB not working in linux. Option ignored" << std::endl;
#else
    FileAttributes attributes = {};
    m_desc.fs->getFileAttributes(s_pdbDir, attributes);
    if (attributes.exists && attributes.isDir)
        m_pdbDirReady = true;
    else if (!attributes.exists)
        m_pdbDirReady = m_desc.fs->carveDirectoryPath(s_pdbDir);

    CPY_ASSERT_FMT(m_pdbDirReady, "Could not created pdb directory %s", s_pdbDir);
    m_createdPdbDir = true;
#endif
}

void BaseShaderDb::setParentDevice(render::IDevice* device, const render::DeviceRuntimeInfo* runtimeInfo)
{
    m_parentDevice = device;
    if (runtimeInfo && (int)runtimeInfo->highestShaderModel < (int)m_desc.shaderModel)
    {
        std::cerr << "WARNING: shader model requested is sm6_" << (int)m_desc.shaderModel
        << " but current graphics card only supports up to sm6_" << (int)runtimeInfo->highestShaderModel
        << ". Forcing max graphics card supported shader model." <<  std::endl;
        m_desc.shaderModel = runtimeInfo->highestShaderModel;
    }
}

BaseShaderDb::~BaseShaderDb()
{
    m_destroying = true;
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

        //onDestroyPayload(*state); //cannot be called becase base VT was destroyed.
        delete state;
    });

    CPY_ASSERT_FMT(unresolvedShaders == 0, "%d unresolved shaders. Expect memory leaks.", unresolvedShaders);
}

void BaseShaderDb::addPath(const char* path)
{
    std::unique_lock lock(m_shadersMutex);
    std::string strpath(path);
    m_additionalPaths.emplace_back(std::move(strpath));
    
    if (m_desc.enableLiveEditing && m_liveEditWatcher)
        m_liveEditWatcher->addDirectory(path);
}

BaseShaderDb::ShaderState& BaseShaderDb::createShaderState(ShaderHandle& outHandle)
{
    ShaderState& shaderState = *(new ShaderState);
    {
        std::unique_lock lock(m_shadersMutex);
        auto& statePtr = m_shaders.allocate(outHandle);
        statePtr = &shaderState;
    }

    shaderState.initialize();
    shaderState.compiling = true;
    return shaderState;
}

ShaderHandle BaseShaderDb::requestCompile(const ShaderDesc& desc)
{
    preparePdbDir();

    auto* compileState = new CompileState;

    std::string filePath = desc.path;

    compileState->compileArgs = {};
    compileState->compileArgs.type = desc.type;
    compileState->compileArgs.shaderModel = m_desc.shaderModel;
    compileState->compileArgs.additionalIncludes = m_additionalPaths;
    compileState->compileArgs.defines = desc.defines;
    compileState->compileArgs.generatePdb = m_pdbDirReady;

    compileState->shaderName = desc.name;
    compileState->mainFn = desc.mainFn;
    compileState->success = false;
    prepareIoJob(*compileState, filePath);
    prepareCompileJobs(*compileState);

    ShaderHandle shaderHandle;
    ShaderState& shaderState = createShaderState(shaderHandle);
    shaderState.debugName = desc.name;
    shaderState.recipe.type = desc.type;
    shaderState.recipe.name = desc.name;
    shaderState.recipe.mainFn = desc.mainFn;
    shaderState.recipe.defines = desc.defines;
    shaderState.recipe.path = compileState->filePath;
    shaderState.compileState = compileState;

    compileState->shaderHandle = shaderHandle;
    m_desc.ts->depends(compileState->compileStep, m_desc.fs->asTask(compileState->readStep));
    m_desc.ts->execute(compileState->compileStep);
    return shaderHandle;
}

ShaderHandle BaseShaderDb::requestCompile(const ShaderInlineDesc& desc)
{
    preparePdbDir();

    auto* compileState = new CompileState;

    compileState->compileArgs = {};
    compileState->shaderName = desc.name;
    compileState->buffer.append((u8*)desc.immCode, strlen(desc.immCode) + 1);
    compileState->mainFn = desc.mainFn;
    compileState->compileArgs.type = desc.type;
    compileState->compileArgs.shaderModel = m_desc.shaderModel;
    compileState->compileArgs.shaderName = compileState->shaderName.c_str();
    compileState->compileArgs.mainFn = compileState->mainFn.c_str();
    compileState->compileArgs.source = (const char*)compileState->buffer.data();
    compileState->compileArgs.sourceSize = (int)compileState->buffer.size();
    compileState->compileArgs.additionalIncludes = m_additionalPaths;
    compileState->compileArgs.defines = desc.defines;
    compileState->compileArgs.generatePdb = m_pdbDirReady;
    compileState->success = false;

    prepareCompileJobs(*compileState);

    ShaderHandle shaderHandle;
    ShaderState& shaderState = createShaderState(shaderHandle);
    shaderState.recipe.type = desc.type;
    shaderState.recipe.name = desc.name;
    shaderState.recipe.mainFn = desc.mainFn;
    shaderState.recipe.defines = desc.defines;
    shaderState.recipe.source = desc.immCode;

    shaderState.debugName = desc.name;
    shaderState.compileState = compileState;
    compileState->shaderHandle = shaderHandle;
    m_desc.ts->execute(compileState->compileStep);
    return shaderHandle;
}

void BaseShaderDb::requestRecompile(ShaderHandle handle)
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
    auto& compileState = *(new CompileState());
    compileState.shaderName = recipe.name;
    compileState.mainFn = recipe.mainFn;
    compileState.compileArgs.type = recipe.type;
    compileState.compileArgs.shaderModel = m_desc.shaderModel;
    compileState.compileArgs.shaderName = recipe.name.c_str();
    compileState.compileArgs.mainFn = recipe.mainFn.c_str();
    compileState.compileArgs.additionalIncludes = m_additionalPaths;
    compileState.compileArgs.defines = recipe.defines;
    compileState.compileArgs.generatePdb = m_pdbDirReady;
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

    shaderState->compileState = &compileState;
    compileState.shaderHandle = handle;
    Task patchTask = m_desc.ts->createTask(TaskDesc(
        [this, &compileState, shaderState](TaskContext& ctx)
        {
            std::unique_lock lock(m_shadersMutex);
            shaderState->compiling = true;
        }));

    m_desc.ts->depends(patchTask, compileState.compileStep);
    compileState.compileStep = patchTask;
    m_desc.ts->execute(compileState.compileStep);
}

void BaseShaderDb::prepareIoJob(CompileState& compileState, const std::string& resolvedPath)
{
    compileState.filePath = resolvedPath;
    compileState.compileArgs.shaderName = compileState.shaderName.c_str();
    compileState.compileArgs.debugName = compileState.filePath.c_str();
    compileState.compileArgs.mainFn = compileState.mainFn.c_str();

    const auto& readPath = compileState.filePath;
    FileReadRequest readRequest(readPath, [&compileState, this](FileReadResponse& response){
        if (response.status == FileStatus::Reading)
        {
            compileState.buffer.append((const u8*)response.buffer, response.size);
        }
        else if (response.status == FileStatus::Success)
        {
            compileState.compileArgs.source = (const char*)compileState.buffer.data();
            compileState.compileArgs.sourceSize = (int)compileState.buffer.size();

            if (m_desc.enableLiveEditing)
            {
                FileLookup fileLookup(response.filePath);
                std::unique_lock lock(m_dependencyMutex);
                m_fileToShaders[fileLookup].insert(compileState.shaderHandle);
                m_shadersToFiles[compileState.shaderHandle].insert(fileLookup);
                compileState.files.insert(response.filePath);
            }
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
    });

    readRequest.additionalRoots.insert(readRequest.additionalRoots.end(), m_additionalPaths.begin(), m_additionalPaths.end());
    compileState.readStep = m_desc.fs->read(readRequest);
}

void BaseShaderDb::prepareCompileJobs(CompileState& compileState)
{
    compileState.compileStep = m_desc.ts->createTask(TaskDesc(
        compileState.shaderName.c_str(),
        [&compileState, this](TaskContext& ctx)
    {
        compileState.success = false;
        if (compileState.compileArgs.source != nullptr)
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
            FileLookup fileLookup(path);
            compileState.files.insert(fileLookup);
        }

        return result;
    };
    
    compileState.compileArgs.onFinished = [&compileState, this](bool success, DxcResultPayload& payload)
    {
        {
            std::unique_lock lock(m_shadersMutex);
            auto& shaderState = m_shaders[compileState.shaderHandle];
            if (success && payload.resultBlob)
            {
                payload.resultBlob->AddRef();
                shaderState->shaderBlob = payload.resultBlob;
                if (payload.spirvReflectionData != nullptr)
                {
                    payload.spirvReflectionData->AddRef();
                    shaderState->spirVReflectionData = payload.spirvReflectionData;
                }
                if (payload.mslData != nullptr)
                {
                    payload.mslData->AddRef();
                    shaderState->mslData = payload.mslData;
                }
            }
            shaderState->ready = true;
            shaderState->success = success;
            compileState.success = success;

        }

        if (success && payload.pdbBlob != nullptr && payload.pdbName != nullptr && m_pdbDirReady)
        {
            std::stringstream ss;
            ss << s_pdbDir << "/";
            ss << ws2s(std::wstring(payload.pdbName->GetStringPointer()));
        
            FileWriteRequest req = {};
            req.doneCallback = [](FileWriteResponse& response) {};
            req.path = ss.str();
            req.buffer = (const char*)payload.pdbBlob->GetBufferPointer();
            req.size = (int)payload.pdbBlob->GetBufferSize();
            AsyncFileHandle writeHandle = m_desc.fs->write(req);
    
            m_desc.fs->execute(writeHandle);
            m_desc.fs->wait(writeHandle);
            m_desc.fs->closeHandle(writeHandle);
        }
    };
}

void BaseShaderDb::resolve(ShaderHandle handle)
{
    CPY_ASSERT(handle.valid());

    if (!handle.valid())
        return;

    ShaderState* shaderState = nullptr;
    {
        std::shared_lock lock(m_shadersMutex);
        shaderState = m_shaders[handle];
    }

    CompileState* compileState = nullptr;
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
            
            if (m_parentDevice != nullptr && shaderState->recipe.type == ShaderType::Compute && compileState->success && !m_destroying)
                onCreateComputePayload(handle, *shaderState);

            delete compileState;

            shaderState->compiling = false;
        }
    }
}

bool BaseShaderDb::isValid(ShaderHandle handle) const
{
    std::shared_lock lock(m_shadersMutex);
    const auto& state = m_shaders[handle];
    return state->ready && state->success;
}

void BaseShaderDb::startLiveEdit()
{
    if  (!m_liveEditWatcher)
        return;

    for (const auto& p : m_additionalPaths)
        m_liveEditWatcher->addDirectory(p.c_str());

    m_liveEditWatcher->addListener(this);
}

void BaseShaderDb::onFilesChanged(const std::set<std::string>& filesChanged)
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
}

void BaseShaderDb::stopLiveEdit()
{
    if (!m_liveEditWatcher)
        return;

    m_liveEditWatcher->removeListener(this);
}

}
