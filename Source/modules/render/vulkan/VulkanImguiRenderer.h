#pragma once

#include "BaseImguiRenderer.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace coalpy
{
namespace render
{

class VulkanDevice;
class VulkanDisplay;

class VulkanImguiRenderer : public BaseImguiRenderer
{
public:
    VulkanImguiRenderer(const IimguiRendererDesc& desc);
    virtual ~VulkanImguiRenderer();

    virtual void newFrame() override;
    virtual void render() override;
    virtual ImTextureID registerTexture(Texture texture) override;
    virtual void unregisterTexture(Texture texture) override;
    virtual bool isTextureRegistered(Texture texture) const override;

private:
    void initPass();
    void initDescriptorPool();
    void initFontTextures();
    void setupSwapChain();
    VulkanDevice& m_device;
    VulkanDisplay& m_display;

    int m_cachedSwapVersion;
    int m_cachedWidth;
    int m_cachedHeight;

    VkRenderPass m_vkRenderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
    VkFramebuffer m_vkFrameBuffer = VK_NULL_HANDLE;
};

}
}
