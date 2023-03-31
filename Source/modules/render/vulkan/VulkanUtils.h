#pragma once

#include <coalpy.render/IDevice.h>
#include <coalpy.core/BitMask.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>

namespace coalpy
{
namespace render
{

template<class T>
T alignByte(T offset, T alignment)
{
	return (offset + (alignment - 1)) & ~(alignment - 1);
}

typedef BitMask VulkanBitMask;

enum class VulkanExtensions : int
{
    SDLExtensions,
    Win32Surface,
    KHRSurface,
    DebugReport,
    CustomBorderColorSampler,
    MinMaxFilterSampler,
    Count
};

enum class VulkanLayers : int
{
    NVOptimus,
    KHRONOSvalidation,
    LUNARGStandardValidation,
    Count
};

enum class VulkanDeviceExtensions : int
{
    KHRSwapChain,
    KHRMaintenance,
    GoogleHLSLFunctionality1,
    GooleUserType,
    DescriptorIndexing,
    MutableDescriptor,
    Count
};

template<typename T>
inline VulkanBitMask asFlag(T v)
{
    return ((VulkanBitMask)1ull) << (int)v;
}

void createVulkanInstance(
    VulkanBitMask requestedLayers,
    VulkanBitMask requestedExtensions,
    VkInstance& outInstance,
    VulkanBitMask& enabledLayers,
    VulkanBitMask& enabledExtensions);

void destroyVulkanInstance(VkInstance instance);

void enumerateVulkanDevices(
    std::vector<DeviceInfo>& outputList, std::vector<VkPhysicalDevice>& vkList);

void createVulkanDevice(
    VkPhysicalDevice physicalDevice,
    VulkanBitMask requestedDeviceExts,
    int queueFamilyIdx,
    VkDevice& outDevice,
    VulkanBitMask& enabledDeviceExts);

int getGraphicsComputeQueueFamilyIndex(VkPhysicalDevice device);

}
}
