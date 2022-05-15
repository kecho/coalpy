#include "VulkanDisplay.h"
#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include <Config.h>
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
    VK_OK(vkCreateSwapchainKHR(m_device.vkDevice(), &swapInfo, nullptr, &m_swapchain))
}

VulkanDisplay::~VulkanDisplay()
{
    destroySwapchain();
    if (m_surface)
        vkDestroySurfaceKHR(m_device.vkInstance(), m_surface, nullptr);
}

Texture VulkanDisplay::texture()
{
    return Texture();
}

void VulkanDisplay::resize(unsigned int width, unsigned int height)
{
    m_config.width = std::max(width, 1u);
    m_config.height = std::max(height, 1u);
    createSwapchain(); //swap chain gets passed trhough old swapchain
}

void VulkanDisplay::present()
{
}

}
}
