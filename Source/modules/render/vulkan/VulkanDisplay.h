#pragma once

#include <coalpy.render/IDisplay.h>
#include <coalpy.render/Resources.h>
#include <vulkan/vulkan.h>
#include "VulkanFencePool.h"

namespace coalpy
{
namespace render
{

class VulkanDevice;

class VulkanDisplay : public IDisplay
{
public:
    VulkanDisplay(const DisplayConfig& config, VulkanDevice& device);
    virtual ~VulkanDisplay();

    virtual Texture texture() override;
    virtual void resize(unsigned int width, unsigned int height) override;
    virtual void present() override;

private:
    void createSwapchain();
    void destroySwapchain();
    void acquireNextImage();
    void waitOnImageFence();

    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VulkanDevice& m_device;
    VkPresentModeKHR m_presentationMode;
    VulkanFenceHandle m_presentFence;
    uint32_t m_activeImageIndex = 0;
    int m_swapCount;
    std::vector<Texture> m_textures;
};

}
}
