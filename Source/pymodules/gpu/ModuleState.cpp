#include "ModuleState.h"
#include <coalpy.core/Assert.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.texture/ITextureLoader.h>
#include "CoalpyTypeObject.h"
#include "Window.h"
#include <string>
#include <iostream>

extern coalpy::ModuleOsHandle g_ModuleInstance;
extern std::string g_ModuleFilePath;

namespace coalpy
{

namespace gpu
{

std::set<ModuleState*> ModuleState::s_allModules;

ModuleState::ModuleState(CoalpyTypeObject** types, int typesCount)
: m_fs(nullptr), m_ts(nullptr), m_textureDestructionCallback(nullptr)
{
    {
        FileWatchDesc desc { 120 /*ms*/};
        m_fw = IFileWatcher::create(desc);
        m_fw->start();
    }

    {
        TaskSystemDesc desc;
        desc.threadPoolSize = 16;
        m_ts = ITaskSystem::create(desc);
    }

    {
        FileSystemDesc desc;
        desc.taskSystem = m_ts;
        m_fs = IFileSystem::create(desc);
    }

    auto flags = render::DeviceFlags::None;
    createDevice(0, (int)flags, ShaderModel::Sm6_5, false);

    {
        m_windowListener = Window::createWindowListener(*this);
    }

    s_allModules.insert(this);
    registerTypes(types, typesCount);
}

void ModuleState::registerTypes(CoalpyTypeObject** types, int typesCount)
{
    for (auto& t : m_types)
        t = nullptr;

    for (int i = 0; i < (int)TypeId::Counts; ++i)
    {
        CoalpyTypeObject* obj = types[i];
        auto typeIndex = (int)obj->typeId;
        if (typeIndex >= (int)TypeId::Counts)
            continue;

        CPY_ASSERT(m_types[typeIndex] == nullptr);
        m_types[typeIndex] = types[i];
        m_types[typeIndex]->moduleState = this;
    }

    for (int i = 0; i < (int)TypeId::Counts; ++i)
        CPY_ASSERT_MSG(m_types[i] != nullptr, "Missing type");
}

bool ModuleState::checkValidDevice()
{
    if (m_device && m_device->info().valid)
        return true;

    PyErr_SetString(exObj(),
        "Current gpu device used is invalid. "
        "Check coalpy.gpu.get_adapters and select "
        "a valid adapter using coalpy.gpu.set_current_adapter.");

    return false;
}

ModuleState::~ModuleState()
{
    if (m_fw)
        m_fw->stop();

    s_allModules.erase(this);
    for (auto w : m_windows)
        w->display = nullptr;

    for (auto* cmdList : m_commandListPool)
        delete cmdList;
    m_commandListPool.clear();

    delete m_windowListener;
    destroyDevice();
    delete m_fs;
    delete m_ts;
    delete m_fw;
}

void ModuleState::startServices()
{
    m_ts->start();
    m_tl->start();
}

void ModuleState::stopServices()
{
    m_ts->signalStop();
    m_ts->join();
}

void ModuleState::addDataPath(const char* dataPath)
{
    std::string p = dataPath;
    auto it = m_dataPathPool.insert(p);
    if (!it.second)
        return;

    m_additionalDataPaths.push_back(p);
    internalAddPath(p);
}

void ModuleState::internalAddPath(const std::string& p)
{
    if (m_db != nullptr)
        m_db->addPath(p.c_str());

    if (m_tl != nullptr)
        m_tl->addPath(p.c_str());
}

bool ModuleState::createDevice(int index, int flags, ShaderModel shaderModel, bool dumpPDBs)
{
    std::string dllname = g_ModuleFilePath;
    std::string modulePath;
    FileUtils::getDirName(dllname, modulePath);
    modulePath += "/resources/";

#if defined(_WIN32)
    auto platform = render::DevicePlat::Dx12;
#elif defined(__linux__)
    auto platform = render::DevicePlat::Vulkan;
#elif
    #error "Platform not supported";
#endif

    {

        ShaderDbDesc desc;
        desc.platform = platform;
        desc.compilerDllPath = modulePath.c_str();
        desc.resolveOnDestruction = true; //this is so there is a nice clean destruction
        desc.fs = m_fs;
        desc.ts = m_ts;
        desc.fw = m_fw;
        desc.enableLiveEditing = true;
        desc.shaderModel = shaderModel;
        desc.dumpPDBs = dumpPDBs;
        desc.onErrorFn = [this](ShaderHandle handle, const char* shaderName, const char* shaderErrorStr)
        {
            onShaderCompileError(handle, shaderName, shaderErrorStr);
        };
        m_db = IShaderDb::create(desc);
    }

    {
        render::DeviceConfig devConfig;
        devConfig.platform = platform;
        devConfig.moduleHandle = g_ModuleInstance;
        devConfig.shaderDb = m_db;
        devConfig.index = index;
        devConfig.flags = (render::DeviceFlags)flags;
        devConfig.resourcePath = modulePath;
#if _DEBUG
        devConfig.flags = (render::DeviceFlags)((int)devConfig.flags | (int)render::DeviceFlags::EnableDebug);
#endif
        m_device = render::IDevice::create(devConfig);
        if (!m_device || !m_device->info().valid)
        {
            PyErr_SetString(exObj(), "Invalid adapter index selected, current gpu device is not valid.");
            return false;
        }
    }

    {
        TextureLoaderDesc desc;
        desc.device = m_device;
        desc.ts = m_ts;
        desc.fs = m_fs;
        desc.fw = m_fw;
        m_tl = ITextureLoader::create(desc);
    }

    for (const auto& p : m_additionalDataPaths)
        internalAddPath(p);

    return true;
}

void ModuleState::destroyDevice()
{
    delete m_tl;
    delete m_device;
    delete m_db;
}

bool ModuleState::selectAdapter(int index, int flags, ShaderModel shaderModel, bool dumpPDBs)
{
    std::vector<render::DeviceInfo> allAdapters;
    render::IDevice::enumerate(render::DevicePlat::Dx12, allAdapters);

    if (allAdapters.empty())
    {
        PyErr_SetString(exObj(), "No graphics card adapter available in the current system.");
        return false;
    }
    if (index < 0 || index >= (int)allAdapters.size())
    {
        PyErr_SetString(exObj(), "Invalid adapter index selected.");
        return false;
    }

    destroyDevice();

    if (!createDevice(index, flags, shaderModel, dumpPDBs))
        return false;

    m_tl->start(); //restart the texture loader
    return true;
}

void ModuleState::onShaderCompileError(ShaderHandle handle, const char* shaderName, const char* shaderErrorString)
{
    std::lock_guard lock(m_shaderErrorMutex);
    std::cerr << "[" << shaderName << "] " << shaderErrorString << std::endl;
}

render::CommandList* ModuleState::newCommandList()
{
    if (m_commandListPool.empty())
    {
        return new render::CommandList();
    }
    else
    {
        auto* cmdList = m_commandListPool.back();
        m_commandListPool.pop_back();
        cmdList->reset();
        return cmdList;
    }
}

void ModuleState::deleteCommandList(render::CommandList* cmdList)
{
    cmdList->reset();
    m_commandListPool.push_back(cmdList);
}

void ModuleState::clean()
{
    for (auto* m : s_allModules)
        delete m;
    
    s_allModules.clear();
}

void ModuleState::onDestroyTexture(Texture& texture)
{
    if (m_textureDestructionCallback == nullptr)
        return;

    m_textureDestructionCallback(texture);
}

}
}
