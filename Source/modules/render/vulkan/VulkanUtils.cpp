#include <Config.h>
#include "VulkanUtils.h"
#if ENABLE_SDL_VULKAN
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif
#include "VulkanQueues.h"
#include <coalpy.core/BitMask.h>

#include <string>
#include <set>
#include <iostream>
#include <functional>

#define DEBUG_PRINT_EXTENSIONS 0

namespace coalpy
{
namespace render
{

//////////////////////////////////////////////
// Extensions, layers and device extensions //
//////////////////////////////////////////////
using ExpansionFn = std::function<void(std::set<std::string>&)>;

void sdlGetExtensions(std::set<std::string>& exts);

struct ExtInfo
{
    const char* extName;
    ExpansionFn extNamesFn;
    ExtInfo(const char* nm) : extName(nm), extNamesFn(nullptr) {}
    ExtInfo(ExpansionFn nmFn) : extName(nullptr), extNamesFn(nmFn) {}
};

static const ExtInfo g_ExtInfos[] = {
    ExtInfo(sdlGetExtensions),
    ExtInfo("VK_KHR_win32_surface"),
    ExtInfo("VK_KHR_surface"),
    ExtInfo(VK_EXT_DEBUG_REPORT_EXTENSION_NAME),
    ExtInfo(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME),
    ExtInfo(VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME)
};
static_assert((int)(sizeof(g_ExtInfos)/sizeof(ExtInfo)) == (int)VulkanExtensions::Count);

static const ExtInfo g_LayerInfos[] = {
    ExtInfo("VK_LAYER_NV_optimus"),
    ExtInfo("VK_LAYER_KHRONOS_validation"),
    ExtInfo("VK_LAYER_LUNARG_standard_validation"),
};
static_assert((int)(sizeof(g_LayerInfos)/sizeof(ExtInfo)) == (int)VulkanLayers::Count);

static const ExtInfo g_DeviceExtInfos[] = {
    ExtInfo(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
    ExtInfo(VK_KHR_MAINTENANCE_1_EXTENSION_NAME),
    ExtInfo(VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME),
    ExtInfo(VK_GOOGLE_USER_TYPE_EXTENSION_NAME),
    ExtInfo(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME),
    ExtInfo(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME),
};
static_assert((int)(sizeof(g_DeviceExtInfos)/sizeof(ExtInfo)) == (int)VulkanDeviceExtensions::Count);
/////////////////////////////////////////////////

struct VkInstanceInfo
{
    int refCount = 0;
    VulkanBitMask enabledLayers;
    VulkanBitMask enabledExtensions;
    std::set<std::string> layerNames;
    VkInstance instance = {};
    VkDebugReportCallbackEXT debugCallback = {};
    
    bool cachedGpuInfos = false;
    std::vector<DeviceInfo> gpus;
    std::vector<VkPhysicalDevice> vkGpus;
};

static VkInstanceInfo g_VkInstanceInfo;

void queryAvailableExtensions(std::set<std::string>& outExtensions)
{
    uint32_t count = 0;
    VK_OK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));

    std::vector<VkExtensionProperties> extensionProps(count);

    VK_OK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensionProps.data()));

    outExtensions.clear();
    for (const auto& props : extensionProps)
        outExtensions.insert(props.extensionName);
}

void queryAvailableLayers(std::set<std::string>& outLayers)
{
    // Figure out the amount of available layers
    // Layers are used for debugging / validation etc / profiling..
    unsigned int instanceLayerCount = 0;
    VK_OK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, NULL));

    std::vector<VkLayerProperties> instanceLayerNames(instanceLayerCount);
    VK_OK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerNames.data()));

    outLayers.clear();
    for (const auto& props : instanceLayerNames)
        outLayers.insert(props.layerName);
}

void queryAvailableDeviceExtensions(VkPhysicalDevice physicalDevice, std::set<std::string>& outExtensions)
{
    uint32_t extCounts = 0;
    VK_OK(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extCounts, NULL))

    std::vector<VkExtensionProperties> extensions(extCounts);
    VK_OK(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extCounts, extensions.data()))

    for (const auto& props : extensions)
        outExtensions.insert(props.extensionName);
}

void sdlGetExtensions(std::set<std::string>& exts)
{
#if ENABLE_SDL_VULKAN
    auto* dummyWindow = SDL_CreateWindow("dummy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    // Figure out the amount of extensions vulkan needs to interface with the os windowing system
    // This is necessary because vulkan is a platform agnostic API and needs to know how to interface with the windowing system
    uint32_t extCount = 0;
    std::vector<const char*> extNames;
    if (dummyWindow == nullptr)
    {
        std::cerr << "SDL could not be initialized" << std::endl;
    }
    else
    {
        if(!SDL_Vulkan_GetInstanceExtensions(dummyWindow, &extCount, nullptr))
        {
            CPY_ERROR_MSG(false, "Unable to query the number of Vulkan instance extensions");
            return;
        }

        extNames.resize(extCount);
        // Use the amount of extensions queried before to retrieve the names of the extensions
        if (!SDL_Vulkan_GetInstanceExtensions(dummyWindow, &extCount, extNames.data()))
        {
            CPY_ERROR_MSG(false, "Unable to query the number of Vulkan instance extension names");
            return;
        }
    }

    exts.insert(extNames.begin(), extNames.end());

    SDL_DestroyWindow(dummyWindow);
#endif
}

void filterLayers(
    VulkanBitMask requestedLayers,
    const ExtInfo* infos,
    int infosCount,
    const std::set<std::string>& filter,
    VulkanBitMask& enabledLayers,
    std::set<std::string>& outputLayers)
{
    enabledLayers = {};

    while (requestedLayers)
    {
        VulkanBitMask mask = (requestedLayers - 1ull) & ~requestedLayers;
        int nextBit = popCnt(mask);
        requestedLayers ^= (1ull << nextBit); 
        CPY_ASSERT(nextBit < infosCount);
        if (nextBit >= infosCount)
            break;

        const ExtInfo& inf = infos[nextBit];
        if (inf.extName != nullptr)
        {
            std::string name(inf.extName);
            if (filter.find(name) != filter.end())
            {
                outputLayers.insert(name);
                enabledLayers |= 1ull << nextBit;
            }
        }
        else
        {
            std::set<std::string> newSet;
            inf.extNamesFn(newSet);
            bool isIncluded = true;
            for (auto& subStr : newSet)
            {
                if (filter.find(subStr) == filter.end())
                {
                    isIncluded = false;
                    break;
                }
            }

            if (isIncluded)
            {
                outputLayers.insert(newSet.begin(), newSet.end());
                enabledLayers |= 1ull << nextBit;
            }
        }
    }
}

void filterVulkanLayers(
    VulkanBitMask requests,
    VulkanBitMask& enabledLayers,
    std::set<std::string>& outputLayers)
{
    std::set<std::string> filter;
    queryAvailableLayers(filter);

    filterLayers(
        requests, g_LayerInfos, (int)VulkanLayers::Count, filter,
        enabledLayers, outputLayers);
}

void filterVulkanExtensions(
    VulkanBitMask requests,
    VulkanBitMask& enabledExt,
    std::set<std::string>& outputExt)
{
    std::set<std::string> filter;
    queryAvailableExtensions(filter);

    filterLayers(
        requests, g_ExtInfos, (int)VulkanExtensions::Count, filter,
        enabledExt, outputExt);
}

void filterVulkanDeviceExtensions(
    VkPhysicalDevice physicalDevice,
    VulkanBitMask requests,
    VulkanBitMask& enabledExt,
    std::set<std::string>& outputExt)
{
    std::set<std::string> filter;
    queryAvailableDeviceExtensions(physicalDevice, filter);

    filterLayers(
        requests, g_DeviceExtInfos, (int)VulkanDeviceExtensions::Count, filter,
        enabledExt, outputExt);
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

void createVulkanInstance(
    VulkanBitMask requestedLayers,
    VulkanBitMask requestedExtensions,
    VkInstance& outInstance,
    VulkanBitMask& enabledLayers,
    VulkanBitMask& enabledExtensions)
{
    if (g_VkInstanceInfo.refCount != 0)
    {
        ++g_VkInstanceInfo.refCount;
        outInstance = g_VkInstanceInfo.instance;
        enabledLayers = g_VkInstanceInfo.enabledLayers;
        enabledExtensions = g_VkInstanceInfo.enabledExtensions;
        if ((enabledLayers & requestedLayers) != enabledLayers)
            std::cerr << "Tried to create a vulkan instance with different vulkan layers requested. " << enabledLayers << ":" << requestedLayers << std::endl;
        if ((enabledExtensions & requestedExtensions) != enabledLayers)
            std::cerr << "Tried to create a vulkan instance with different vulkan extensions requested." << enabledExtensions << ":" << requestedExtensions << std::endl;
        return;
    }

#if ENABLE_SDL_VULKAN
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    CPY_ASSERT_MSG(ret == 0, "Failed initializing SDL2");
#endif

    std::set<std::string> layerNamesSet;
    filterVulkanLayers(requestedLayers, enabledLayers, layerNamesSet);

    std::set<std::string> extNamesSet;
    filterVulkanExtensions(requestedExtensions, enabledExtensions, extNamesSet);

    unsigned int apiVersion;
    vkEnumerateInstanceVersion(&apiVersion);

    std::vector<const char*> layerNames;
    for (const auto& layer : layerNamesSet)
        layerNames.emplace_back(layer.c_str());

    std::vector<const char*> extNames;
    for (const auto& ext : extNamesSet)
        extNames.emplace_back(ext.c_str());

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "coalpy";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "coalpy";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = NULL;
    instInfo.flags = 0;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = static_cast<uint32_t>(extNames.size());
    instInfo.ppEnabledExtensionNames = extNames.data();
    instInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    instInfo.ppEnabledLayerNames = layerNames.data();

    VkResult res = vkCreateInstance(&instInfo, NULL, &outInstance);
    CPY_ASSERT_MSG(res == VK_SUCCESS, "Failed creating vulkan instance");
    if (res != VK_SUCCESS)
    {
        std::cerr << "unable to create vulkan instance, error\n" << res;
        return;
    }

    if ((asFlag(VulkanExtensions::DebugReport) & enabledExtensions) != 0)
        setupDebugCallback(outInstance, g_VkInstanceInfo.debugCallback);

    g_VkInstanceInfo.instance = outInstance;
    g_VkInstanceInfo.layerNames = layerNamesSet;
    g_VkInstanceInfo.enabledLayers = enabledLayers;
    g_VkInstanceInfo.enabledExtensions = enabledExtensions;
    ++g_VkInstanceInfo.refCount;
    cacheGPUDevices();
}

void destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

void destroyVulkanInstance(VkInstance instance)
{
    CPY_ASSERT(instance == g_VkInstanceInfo.instance);
    --g_VkInstanceInfo.refCount;
    if (g_VkInstanceInfo.refCount == 0)
    {
        if ((asFlag(VulkanExtensions::DebugReport) & g_VkInstanceInfo.enabledExtensions) != 0)
            destroyDebugReportCallbackEXT(instance, g_VkInstanceInfo.debugCallback, nullptr);
        vkDestroyInstance(instance, nullptr);
        g_VkInstanceInfo = {};
    }
}

void destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func == nullptr)
        return;
    func(instance, callback, pAllocator);
}

void enumerateVulkanDevices(std::vector<DeviceInfo>& outputList, std::vector<VkPhysicalDevice>& outVkList)
{
    VkInstance instance = g_VkInstanceInfo.instance;
    bool createDummyInstance = g_VkInstanceInfo.refCount == 0;
    if (createDummyInstance)
    {
        VulkanBitMask dummy0 = 0, dummy1 = 0;
        createVulkanInstance(0, 0, instance, dummy0, dummy1);
    }
    else
        instance = g_VkInstanceInfo.instance;

    if (instance == VK_NULL_HANDLE)
        return;

    cacheGPUDevices();

    outputList = g_VkInstanceInfo.gpus;

    if (createDummyInstance)
        destroyVulkanInstance(instance);
    else
        outVkList = g_VkInstanceInfo.vkGpus;
}

void createVulkanDevice(
    VkPhysicalDevice physicalDevice,
    VulkanBitMask requestedDeviceExts,
    int queueFamilyIdx,
    VkDevice& outDevice,
    VulkanBitMask& enabledDeviceExts)
{
    enabledDeviceExts = {};
    outDevice = VK_NULL_HANDLE;
    std::vector<const char*> layerNames;
    for (const auto& layer : g_VkInstanceInfo.layerNames)
        layerNames.emplace_back(layer.c_str());

    std::set<std::string> deviceExts;
    filterVulkanDeviceExtensions(
        physicalDevice, requestedDeviceExts,
        enabledDeviceExts, deviceExts);

    std::vector<const char*> deviceExtensionNames;
    for (const auto& extName : deviceExts)
        deviceExtensionNames.emplace_back(extName.c_str());

    // Create queue information structure used by device based on the previously fetched queue information from the physical device
    // We create one command processing queue for graphics
    VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr };
    queueCreateInfo.queueFamilyIndex = queueFamilyIdx;
    queueCreateInfo.queueCount = (int)WorkType::Count;
    std::vector<float> queuePrio = { 1.0f };
    queueCreateInfo.pQueuePriorities = queuePrio.data();
    queueCreateInfo.flags = 0;

    VkPhysicalDeviceVulkan12Features vulkanFeatures12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, nullptr };
    VkPhysicalDeviceFeatures2 vulkanFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &vulkanFeatures12 };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &vulkanFeatures2);

    // Device creation information
    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.ppEnabledLayerNames = layerNames.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    createInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size());
    createInfo.pEnabledFeatures = NULL;
    createInfo.flags = 0;
    createInfo.pNext = &vulkanFeatures2;

    VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE valveF = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE, nullptr };
    VkPhysicalDeviceDescriptorIndexingFeatures indexingF = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, nullptr };
    if ((enabledDeviceExts & asFlag(VulkanDeviceExtensions::MutableDescriptor)) != 0)
    {
        valveF.mutableDescriptorType = true;
        valveF.pNext = const_cast<void*>(createInfo.pNext);
        createInfo.pNext = &valveF;
    }

    if ((enabledDeviceExts & asFlag(VulkanDeviceExtensions::DescriptorIndexing)) != 0)
    {
        indexingF.descriptorBindingPartiallyBound = true;
        indexingF.runtimeDescriptorArray = true;
        indexingF.pNext = const_cast<void*>(createInfo.pNext);
        createInfo.pNext = &indexingF;
    }

    // Finally we're ready to create a new device
    VK_OK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &outDevice));
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
           (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 &&
           (props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
            return i;
    }

    return -1;
}


}
}
