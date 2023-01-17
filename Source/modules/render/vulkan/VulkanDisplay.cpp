#include <Config.h>
#if ENABLE_WIN_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#endif
#include "VulkanDisplay.h"
#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanQueues.h"
#include <algorithm>
#include <iostream>

#if ENABLE_SDL_VULKAN
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif

namespace coalpy
{
namespace render
{

namespace
{

bool getSurfaceProperties(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR& capabilities)
{
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities) != VK_SUCCESS)
    {
        std::cerr << "unable to acquire surface capabilities\n";
        return false;
    }
    return true;
}

bool getPresentationMode(VkSurfaceKHR surface, VkPhysicalDevice device, VkPresentModeKHR& ioMode)
{
    uint32_t modeCount(0);
    if(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, NULL) != VK_SUCCESS)
    {
        std::cerr << "unable to query present mode count for physical device" << std::endl;
        return false;
    }

    std::vector<VkPresentModeKHR> availableModes(modeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, availableModes.data()) != VK_SUCCESS)
    {
        std::cerr << "unable to query the various present modes for physical device" << std::endl;
        return false;
    }

    for (auto& mode : availableModes)
    {
        if (mode == ioMode)
            return true;
    }

    std::cerr << "unable to obtain preferred display mode, fallback to FIFO" << std::endl;
    ioMode = VK_PRESENT_MODE_FIFO_KHR;
    return true;
}

}

VulkanDisplay::VulkanDisplay(const DisplayConfig& config, VulkanDevice& device)
: IDisplay(config), m_device(device), m_swapCount(0)
{
    m_surface = {};
    m_swapchain = {};
    m_presentationMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
#if ENABLE_SDL_VULKAN
    auto* window = (SDL_Window*)config.handle;
    if (!SDL_Vulkan_CreateSurface(window, device.vkInstance(), &m_surface))
        std::cerr << "Unable to create Vulkan compatible surface using SDL\n";

    // Make sure the surface is compatible with the queue family and gpu
    VkBool32 supported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device.vkPhysicalDevice(), device.graphicsFamilyQueueIndex(), m_surface, &supported);
    if (!supported)
    {
        std::cerr << "Surface is not supported by physical device!\n";
    }

#elif ENABLE_WIN_VULKAN
    VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr };
    createInfo.hwnd = (HWND)config.handle;
    createInfo.hinstance = GetModuleHandle(nullptr);
    if (vkCreateWin32SurfaceKHR(m_device.vkInstance(), &createInfo, nullptr, &m_surface) != VK_SUCCESS)
        std::cerr << "Windows vulkan surface is not supported by physical device!\n";
#endif

    if (!m_surface)
        return;

    VkSurfaceCapabilitiesKHR surfaceProperties;
    if (!getSurfaceProperties(m_device.vkPhysicalDevice(), m_surface, surfaceProperties))
    {
        std::cerr << "No present surface capabilities found." << std::endl;
        return;
    }

    m_swapCount = std::clamp(config.buffering, surfaceProperties.minImageCount, surfaceProperties.maxImageCount);

    if (!getPresentationMode(m_surface, m_device.vkPhysicalDevice(), m_presentationMode))
    {
        std::cerr << "Failed setting presentation mode " << m_presentationMode << " on surface." << std::endl;
        return;
    }

    createSwapchain();
}

void VulkanDisplay::destroySwapchain()
{
    if (!m_swapchain)
        return;

    vkDestroySwapchainKHR(m_device.vkDevice(), m_swapchain, nullptr);
    m_swapchain = {};
}

void VulkanDisplay::createSwapchain()
{
    if (m_swapchain)
    {
        waitOnImageFence();
        VulkanFencePool& fencePool = m_device.fencePool();
        auto fenceVal = fencePool.allocate();
        VK_OK(vkQueueSubmit(m_device.queues().cmdQueue(WorkType::Graphics), 0, nullptr, fencePool.get(fenceVal)));
        fencePool.waitOnCpu(fenceVal);
    }
    
    VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    VkExtent2D extent = { m_config.width, m_config.height };
    // Get swapchain image format
    VkFormat imageFormat = getVkFormat(m_config.format);

    // Populate swapchain creation info
    VkSwapchainCreateInfoKHR swapInfo = {};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.flags = 0;
    swapInfo.surface = m_surface;
    swapInfo.minImageCount = m_swapCount;
    swapInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    swapInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.queueFamilyIndexCount = 0;
    swapInfo.pQueueFamilyIndices = nullptr;
    swapInfo.preTransform = transform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = m_presentationMode;
    swapInfo.clipped = true;
    swapInfo.oldSwapchain = m_swapchain;
    m_swapchain = {};
    auto ret = vkCreateSwapchainKHR(m_device.vkDevice(), &swapInfo, nullptr, &m_swapchain);
    VK_OK(ret);

    uint32_t imageCounts = 0;
    VK_OK(vkGetSwapchainImagesKHR(m_device.vkDevice(), m_swapchain, &imageCounts, nullptr));

    m_vkImages.clear();
    m_vkImages.resize(imageCounts);
    VK_OK(vkGetSwapchainImagesKHR(m_device.vkDevice(), m_swapchain, &imageCounts, m_vkImages.data()));

    acquireNextImage();
}

VulkanDisplay::~VulkanDisplay()
{
    destroySwapchain();
    if (m_surface)
        vkDestroySurfaceKHR(m_device.vkInstance(), m_surface, nullptr);

    if (m_presentFence.valid())
        m_device.fencePool().free(m_presentFence);
}

Texture VulkanDisplay::texture()
{
    return Texture();
}

void VulkanDisplay::resize(unsigned int width, unsigned int height)
{
    m_config.width = width == 0 ? 1u : width;
    m_config.height = height == 0 ? 1u : height;
    createSwapchain(); //swap chain gets passed trhough old swapchain
}

void VulkanDisplay::acquireNextImage()
{
    VulkanFencePool& fencePool = m_device.fencePool();
    CPY_ASSERT(!m_presentFence.valid());
    m_presentFence = fencePool.allocate();
    VkAcquireNextImageInfoKHR acquireImageInfo = { VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR, nullptr };
    acquireImageInfo.swapchain = m_swapchain;
    acquireImageInfo.fence = fencePool.get(m_presentFence);
    acquireImageInfo.deviceMask = 1;
    VK_OK(vkAcquireNextImage2KHR(m_device.vkDevice(), &acquireImageInfo, &m_activeImageIndex));
}

void VulkanDisplay::waitOnImageFence()
{
    if (!m_presentFence.valid())
        return;
     
    VulkanFencePool& fencePool = m_device.fencePool();
    fencePool.updateState(m_presentFence);
    if (!fencePool.isSignaled(m_presentFence))
        fencePool.waitOnCpu(m_presentFence);
    
    fencePool.free(m_presentFence);
    m_presentFence = VulkanFenceHandle();
}

void VulkanDisplay::present()
{
    waitOnImageFence();

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &m_activeImageIndex;

    VkQueue queue = m_device.queues().cmdQueue(WorkType::Graphics);
    VK_OK(vkQueuePresentKHR(queue, &presentInfo));

    acquireNextImage();
}

}
}
