#pragma once

#include <coalpy.render/IDisplay.h>
#include <vulkan/vulkan.h>

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

    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VulkanDevice& m_device;
    VkPresentModeKHR m_presentationMode;
    int m_swapCount;
};

}
}
