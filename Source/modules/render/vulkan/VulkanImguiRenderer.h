#pragma once

#include "BaseImguiRenderer.h"
#include "VulkanFencePool.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <coalpy.render/Resources.h>
#include <unordered_map>

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
    void initSampler();
    void initDescriptorPool();
    void initFontTextures();
    void setupSwapChain();
    void flushGarbage();
    VulkanDevice& m_device;
    VulkanDisplay& m_display;

    int m_cachedSwapVersion;
    int m_cachedWidth;
    int m_cachedHeight;

    VkRenderPass m_vkRenderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
    VkFramebuffer m_vkFrameBuffer = VK_NULL_HANDLE;
    Sampler m_sampler;

    struct GarbageObject
    {
        VulkanFenceHandle fence;
        VkDescriptorSet textureDescriptor;
    };

    std::vector<GarbageObject> m_garbageTextures;
    std::unordered_map<Texture, VkDescriptorSet> m_textures;
};

}
}
