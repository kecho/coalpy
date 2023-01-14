#include "ModuleState.h"
#include <coalpy.core/Assert.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.render/ShaderModels.h>
#include <coalpy.texture/ITextureLoader.h>
#include "CoalpyTypeObject.h"
#include "Window.h"
#include "SettingsSchema.h"
#include "ModuleSettings.h"
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
    registerTypes(types, typesCount);

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

    m_ts->start();

    loadSettings();

    {
        m_windowListener = Window::createWindowListener(*this);
    }

    s_allModules.insert(this);
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

void ModuleState::loadSettings()
{
    m_settings = alloc<ModuleSettings>();
    new (m_settings) ModuleSettings;
    m_settings->load(*m_fs, ModuleSettings::sSettingsFileName);
}

bool ModuleState::checkValidDevice()
{
    if (m_device && m_device->info().valid)
        return true;
    
    if (createDeviceFromSettings())
        return true;

    PyErr_SetString(exObj(),
        "Current gpu device used is invalid. "
        "Check coalpy.gpu.get_adapters and select "
        "a valid adapter using coalpy.gpu.set_current_adapter.");

    return false;
}

static ShaderModel strToShaderModel(const std::string& sm)
{
    if (sm == "sm6_5")
        return ShaderModel::Sm6_5;
    if (sm == "sm6_4")
        return ShaderModel::Sm6_4;
    if (sm == "sm6_3")
        return ShaderModel::Sm6_3;
    if (sm == "sm6_2")
        return ShaderModel::Sm6_2;
    if (sm == "sm6_1")
        return ShaderModel::Sm6_1;
    if (sm == "sm6_0")
        return ShaderModel::Sm6_0;

    return ShaderModel::Sm6_5;
}

bool ModuleState::createDeviceFromSettings()
{
    std::vector<render::DeviceInfo> allAdapters;
#if defined(_WIN32)
    auto platform = render::DevicePlat::Dx12;
#elif defined(__linux__)
    auto platform = render::DevicePlat::Vulkan;
#elif
    #error "Platform not supported";
#endif
    render::IDevice::enumerate(platform, allAdapters);

    int index = m_settings->adapter_index < 0 ? 0 : m_settings->adapter_index;
    if (allAdapters.empty())
    {
        PyErr_SetString(exObj(), "No graphics card adapter available in the current system.");
        return false;
    }
    if (index >= (int)allAdapters.size())
    {
        PyErr_SetString(exObj(), "Invalid adapter index selected.");
        return false;
    }
    
    int flags = (int)render::DeviceFlags::None;
    if (m_settings->enable_debug_device)
        flags |= (int)render::DeviceFlags::EnableDebug;
    return createDevice(m_settings->adapter_index, flags, strToShaderModel(m_settings->shader_model), m_settings->dump_shader_pdbs);
}

ModuleState::~ModuleState()
{
    Py_DECREF(m_settings);

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

    if (!strcmpi(m_settings->graphics_api.c_str(), "dx12"))
        platform = render::DevicePlat::Dx12;
    else if (!strcmpi(m_settings->graphics_api.c_str(), "vulkan"))
        platform = render::DevicePlat::Vulkan;
    else if (strcmpi(m_settings->graphics_api.c_str(), "default"))
    {
        PyErr_Format(exObj(), "Unrecognized setting for graphics API \"%s\" Default will be used: %s", m_settings->graphics_api.c_str(), render::getDevicePlatName(platform));
        return false;
    }

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

#if _DEBUG
        flags |= (int)render::DeviceFlags::EnableDebug;
#endif
        render::DeviceConfig devConfig;
        devConfig.platform = platform;
        devConfig.moduleHandle = g_ModuleInstance;
        devConfig.shaderDb = m_db;
        devConfig.index = index;
        devConfig.flags = (render::DeviceFlags)flags;
        devConfig.resourcePath = modulePath;
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

    startServices();

    return true;
}

void ModuleState::destroyDevice()
{
    if (m_device)
        m_device->removeShaderDb();
    delete m_tl;
    delete m_db;
    if (m_device)
        delete m_device;
    m_device = nullptr;
    m_tl = nullptr;
    m_db = nullptr;
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
