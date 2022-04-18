#include <Config.h>
#if ENABLE_VULKAN

#include "VulkanDevice.h"
#include "VulkanShaderDb.h"
#if ENABLE_SDL_VULKAN
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif
#include <iostream>
#include <set>
#include <vector>

namespace coalpy
{
namespace render
{

namespace 
{

struct VkInstanceInfo
{
    int refCount = 0;
    std::vector<std::string> layerNames;
    std::vector<std::string> extensionNames;
    VkInstance instance = {};
    
    bool cachedGpuInfos = false;
    std::vector<DeviceInfo> gpus;
    std::vector<VkPhysicalDevice> vkGpus;
} g_VkInstanceInfo;

void destroyVulkanInstance(VkInstance instance)
{
    CPY_ASSERT(instance == g_VkInstanceInfo.instance);
    --g_VkInstanceInfo.refCount;
    if (g_VkInstanceInfo.refCount == 0)
    {
        vkDestroyInstance(instance, nullptr);
        g_VkInstanceInfo = {};
    }
}

const std::set<std::string>& getRequestedLayerNames()
{
    static std::set<std::string> layers;
    if (layers.empty())
    {
        layers.emplace("VK_LAYER_NV_optimus");
        layers.emplace("VK_LAYER_KHRONOS_validation");
    }
    return layers;
}

const std::set<std::string>& getRequestedDeviceExtensionNames()
{
    static std::set<std::string> layers;
    if (layers.empty())
    {
        layers.emplace(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    return layers;
}

bool getAvailableVulkanExtensions(std::vector<std::string>& outExtensions)
{
    unsigned int extCount = 0;
#if ENABLE_SDL_VULKAN
    auto* dummyWindow = SDL_CreateWindow("dummy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);

    // Figure out the amount of extensions vulkan needs to interface with the os windowing system
    // This is necessary because vulkan is a platform agnostic API and needs to know how to interface with the windowing system
    if (!SDL_Vulkan_GetInstanceExtensions(dummyWindow, &extCount, nullptr))
    {
        CPY_ERROR_MSG(false, "Unable to query the number of Vulkan instance extensions");
        return false;
    }

    std::vector<const char*> extNames(extCount);
    // Use the amount of extensions queried before to retrieve the names of the extensions
    if (!SDL_Vulkan_GetInstanceExtensions(dummyWindow, &extCount, extNames.data()))
    {
        CPY_ERROR_MSG(false, "Unable to query the number of Vulkan instance extension names");
        return false;
    }

    // Display names
    for (unsigned int i = 0; i < extCount; i++)
        outExtensions.emplace_back(extNames[i]);
#endif


    // Add debug display extension, we need this to relay debug messages
    outExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

#if ENABLE_SDL_VULKAN
    SDL_DestroyWindow(dummyWindow);
#endif
    return true;
}

bool getAvailableVulkanLayers(std::vector<std::string>& outLayers)
{
    // Figure out the amount of available layers
    // Layers are used for debugging / validation etc / profiling..
    unsigned int instanceLayerCount = 0;
    VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerCount, NULL);
    if (res != VK_SUCCESS)
    {
        CPY_ASSERT_MSG(false, "unable to query vulkan instance layer property count");
        return false;
    }

    std::vector<VkLayerProperties> instanceLayerNames(instanceLayerCount);
    res = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerNames.data());
    if (res != VK_SUCCESS)
    {
        CPY_ASSERT_MSG(false, "unable to retrieve vulkan instance layer names");
        return false;
    }

    std::vector<const char*> valid_instanceLayerNames;
    const std::set<std::string>& lookup_layers = getRequestedLayerNames();
    int count(0);
    outLayers.clear();
    for (const auto& name : instanceLayerNames)
    {
        auto it = lookup_layers.find(std::string(name.layerName));
        if (it != lookup_layers.end())
            outLayers.emplace_back(name.layerName);
        count++;
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    std::cerr << "layer: " << layerPrefix << ": " << msg << std::endl;
    return VK_FALSE;
}

VkResult createDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

bool setupDebugCallback(VkInstance instance, VkDebugReportCallbackEXT& callback)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    if (createDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
    {
        CPY_ASSERT_MSG(false, "Unable to create debug report callback extension\n");
        return false;
    }
    return true;
}

void destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
    {
        func(instance, callback, pAllocator);
    }
}

void cacheGPUDevices()
{
    if (g_VkInstanceInfo.cachedGpuInfos)
        return;

    CPY_ASSERT(g_VkInstanceInfo.refCount != 0);
    VkInstance instance = g_VkInstanceInfo.instance;
    unsigned int gpuCounts = 0;
    VK_OK(vkEnumeratePhysicalDevices(instance, &gpuCounts, nullptr));
    if (gpuCounts == 0)
    {
        std::cerr << "No vulkan compatible GPU Devices found." << std::endl;
        return;
    }

    std::vector<VkPhysicalDevice> gpus(gpuCounts);
    VK_OK(vkEnumeratePhysicalDevices(instance, &gpuCounts, gpus.data()));

    int index = 0;
    for (const auto& gpu : gpus)
    {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(gpu, &props);

        DeviceInfo deviceInfo;
        deviceInfo.index = index++;
        deviceInfo.valid = true;
        deviceInfo.name = props.deviceName;

        g_VkInstanceInfo.gpus.push_back(deviceInfo);
    }

    g_VkInstanceInfo.vkGpus = std::move(gpus);
    g_VkInstanceInfo.cachedGpuInfos = true;
}

bool createVulkanInstance(VkInstance& outInstance)
{
    if (g_VkInstanceInfo.refCount != 0)
    {
        ++g_VkInstanceInfo.refCount;
        outInstance = g_VkInstanceInfo.instance;
        return true;
    }

#if ENABLE_SDL_VULKAN
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    CPY_ASSERT_MSG(ret == 0, "Failed initializing SDL2");
#endif

    bool retSuccess;
    retSuccess = getAvailableVulkanLayers(g_VkInstanceInfo.layerNames);
    CPY_ASSERT_MSG(retSuccess, "Failed getting available vulkan layers.");

    retSuccess = getAvailableVulkanExtensions(g_VkInstanceInfo.extensionNames);
    CPY_ASSERT_MSG(retSuccess, "Failed getting vulkan extensions.");

    unsigned int apiVersion;
    vkEnumerateInstanceVersion(&apiVersion);

    std::vector<const char*> layerNames;
    for (const auto& layer : g_VkInstanceInfo.layerNames)
        layerNames.emplace_back(layer.c_str());

    std::vector<const char*> extNames;
    for (const auto& ext : g_VkInstanceInfo.extensionNames)
        extNames.emplace_back(ext.c_str());

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "coalpy";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "coalpy";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = NULL;
    instInfo.flags = 0;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = static_cast<uint32_t>(extNames.size());
    instInfo.ppEnabledExtensionNames = extNames.data();
    instInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    instInfo.ppEnabledLayerNames = layerNames.data();

    VkResult res = vkCreateInstance(&instInfo, NULL, &g_VkInstanceInfo.instance);
    CPY_ASSERT_MSG(res == VK_SUCCESS, "Failed creating vulkan instance");
    if (res != VK_SUCCESS)
    {
        std::cerr << "unable to create vulkan instance, error\n" << res;
        return false;
    }

    ++g_VkInstanceInfo.refCount;
    cacheGPUDevices();
    return true;
}

int getGraphicsComputeQueueFamilyIndex(VkPhysicalDevice device)
{
    unsigned int famCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &famCount, nullptr);

    std::vector<VkQueueFamilyProperties> famProps(famCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &famCount, famProps.data());


    for (int i = 0; i < (int)famProps.size(); ++i)
    {
        const VkQueueFamilyProperties& props = famProps[i];
        if (props.queueCount > 0 &&
           (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
           (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
            return i;
    }

    return -1;
}

VkDevice createVkDevice(
    VkPhysicalDevice physicalDevice,
    int queueFamilyIdx)
{
    std::vector<const char*> layerNames;
    for (const auto& layer : g_VkInstanceInfo.layerNames)
        layerNames.emplace_back(layer.c_str());

    // Get the number of available extensions for our graphics card
    uint32_t devicePropertyCount = 0;
    VK_OK(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &devicePropertyCount, NULL))

    // Acquire their actual names
    std::vector<VkExtensionProperties> deviceProperties(devicePropertyCount);
    VK_OK(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &devicePropertyCount, deviceProperties.data()))

    // Match names against requested extension
    std::vector<const char*> devicePropertyNames;
    const std::set<std::string>& requiredExtensionNames = getRequestedDeviceExtensionNames();
    for (const auto& extProperty : deviceProperties)
    {
        auto it = requiredExtensionNames.find(std::string(extProperty.extensionName));
        if (it != requiredExtensionNames.end())
            devicePropertyNames.emplace_back(extProperty.extensionName);
    }

    // Create queue information structure used by device based on the previously fetched queue information from the physical device
    // We create one command processing queue for graphics
    VkDeviceQueueCreateInfo queueCreateInfo;
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIdx;
    queueCreateInfo.queueCount = 1;
    std::vector<float> queuePrio = { 1.0f };
    queueCreateInfo.pQueuePriorities = queuePrio.data();
    queueCreateInfo.pNext = NULL;
    queueCreateInfo.flags = 0;

    // Device creation information
    VkDeviceCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.ppEnabledLayerNames = layerNames.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    createInfo.ppEnabledExtensionNames = devicePropertyNames.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(devicePropertyNames.size());
    createInfo.pNext = NULL;
    createInfo.pEnabledFeatures = NULL;
    createInfo.flags = 0;

    VkDevice outDevice = {};
    // Finally we're ready to create a new device
    VK_OK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &outDevice));

    return outDevice;
}

}

VulkanDevice::VulkanDevice(const DeviceConfig& config)
: TDevice<VulkanDevice>(config),
  m_shaderDb(nullptr)
{
    createVulkanInstance(m_vkInstance);
    int selectedDeviceIdx = std::min<int>(std::max<int>(config.index, 0), (int)g_VkInstanceInfo.vkGpus.size() - 1);
    m_info = g_VkInstanceInfo.gpus[selectedDeviceIdx];
    m_vkPhysicalDevice = g_VkInstanceInfo.vkGpus[selectedDeviceIdx];

    int queueFamIndex = getGraphicsComputeQueueFamilyIndex(m_vkPhysicalDevice);
    CPY_ASSERT(queueFamIndex != -1);

    m_vkDevice = createVkDevice(m_vkPhysicalDevice, queueFamIndex);

    if (queueFamIndex == -1)
        std::cerr << "Could not find a compute queue for device selected" << std::endl;

    if (config.shaderDb)
    {
        m_shaderDb = static_cast<VulkanShaderDb*>(config.shaderDb);
        CPY_ASSERT_MSG(m_shaderDb->parentDevice() == nullptr, "shader database can only belong to 1 and only 1 device");
        m_shaderDb->setParentDevice(this);
    }
}

VulkanDevice::~VulkanDevice()
{
    if (m_shaderDb && m_shaderDb->parentDevice() == this)
        m_shaderDb->setParentDevice(nullptr);

    vkDestroyDevice(m_vkDevice, nullptr); 
    destroyVulkanInstance(m_vkInstance);    
}

void VulkanDevice::enumerate(std::vector<DeviceInfo>& outputList)
{
    VkInstance instance = g_VkInstanceInfo.instance;
    if (g_VkInstanceInfo.refCount == 0 && !createVulkanInstance(instance))
        return;

    cacheGPUDevices();

    outputList = g_VkInstanceInfo.gpus;
}

TextureResult VulkanDevice::createTexture(const TextureDesc& desc)
{
    return TextureResult();
}

TextureResult VulkanDevice::recreateTexture(Texture texture, const TextureDesc& desc)
{
    return TextureResult();
}

BufferResult  VulkanDevice::createBuffer (const BufferDesc& config)
{
    return BufferResult();
}

SamplerResult VulkanDevice::createSampler (const SamplerDesc& config)
{
    return SamplerResult();
}

InResourceTableResult VulkanDevice::createInResourceTable  (const ResourceTableDesc& config)
{
    return InResourceTableResult();
}

OutResourceTableResult VulkanDevice::createOutResourceTable (const ResourceTableDesc& config)
{
    return OutResourceTableResult();
}

SamplerTableResult  VulkanDevice::createSamplerTable (const ResourceTableDesc& config)
{
    return SamplerTableResult();
}

void VulkanDevice::getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo)
{
    memInfo = ResourceMemoryInfo();
}

WaitStatus VulkanDevice::waitOnCpu(WorkHandle handle, int milliseconds)
{
    return WaitStatus();
}

DownloadStatus VulkanDevice::getDownloadStatus(WorkHandle bundle, ResourceHandle handle, int mipLevel, int arraySlice)
{
    return DownloadStatus();
}

void VulkanDevice::release(ResourceHandle resource)
{
}

void VulkanDevice::release(ResourceTable table)
{
}

SmartPtr<IDisplay> VulkanDevice::createDisplay(const DisplayConfig& config)
{
    return nullptr;
}

void VulkanDevice::internalReleaseWorkHandle(WorkHandle handle)
{
}

ScheduleStatus VulkanDevice::internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle)
{
    return ScheduleStatus();
}

}
}

#endif
