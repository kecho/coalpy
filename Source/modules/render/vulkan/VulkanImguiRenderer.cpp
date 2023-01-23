#include "VulkanImguiRenderer.h"
#include "Config.h"
#include "VulkanDisplay.h"
#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanQueues.h"
#include "VulkanResources.h"
#include "VulkanFencePool.h"
#include "VulkanBarriers.h"

#include <backends/imgui_impl_vulkan.h>

namespace coalpy
{
namespace render
{

VulkanImguiRenderer::VulkanImguiRenderer(const IimguiRendererDesc& desc)
: BaseImguiRenderer(desc)
, m_device(*(VulkanDevice*)desc.device)
, m_display(*(VulkanDisplay*)desc.display)
, m_cachedSwapVersion(-1)
, m_cachedWidth(-1)
, m_cachedHeight(-1)
{
    initPass();
    initDescriptorPool();

    ImGui_ImplVulkan_InitInfo vulkanInitInfo = {};
    vulkanInitInfo.Instance = m_device.vkInstance();
    vulkanInitInfo.PhysicalDevice = m_device.vkPhysicalDevice();
    vulkanInitInfo.Device = m_device.vkDevice();
    vulkanInitInfo.QueueFamily = m_device.graphicsFamilyQueueIndex();
    vulkanInitInfo.Queue = m_device.queues().cmdQueue(WorkType::Graphics);
    vulkanInitInfo.DescriptorPool = m_vkDescriptorPool;
    vulkanInitInfo.Subpass = 0;
    vulkanInitInfo.MinImageCount = m_display.config().buffering;
    vulkanInitInfo.ImageCount = m_display.config().buffering;
    vulkanInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&vulkanInitInfo, m_vkRenderPass);

    initFontTextures();
    setupSwapChain();
}

VulkanImguiRenderer::~VulkanImguiRenderer()
{
    activate();
    VulkanFencePool& fencePool = m_device.fencePool();
    VkQueue queue = m_device.queues().cmdQueue(WorkType::Graphics);
    VulkanFenceHandle fenceHandle = fencePool.allocate();

    vkQueueSubmit(queue, 0, nullptr, fencePool.get(fenceHandle));

    fencePool.waitOnCpu(fenceHandle);
    fencePool.free(fenceHandle);

    ImGui_ImplVulkan_Shutdown();

    if (m_vkFrameBuffer)
        vkDestroyFramebuffer(m_device.vkDevice(), m_vkFrameBuffer, nullptr) ;
    if (m_vkDescriptorPool)
        vkDestroyDescriptorPool(m_device.vkDevice(), m_vkDescriptorPool, nullptr);
    if (m_vkRenderPass)
        vkDestroyRenderPass(m_device.vkDevice(), m_vkRenderPass, nullptr);
}

void VulkanImguiRenderer::initFontTextures()
{
    VulkanQueues& queues = m_device.queues();
    VulkanFencePool& fencePool = m_device.fencePool();
    VulkanFenceHandle fenceHandle = fencePool.allocate();

    VkQueue queue = queues.cmdQueue(WorkType::Graphics);
    VulkanList list;
    queues.allocate(WorkType::Graphics, list);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(list.list, &beginInfo);
    ImGui_ImplVulkan_CreateFontsTexture(list.list);
    vkEndCommandBuffer(list.list);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &list.list;

    VK_OK(vkQueueSubmit(queue, 1u, &submitInfo, fencePool.get(fenceHandle)));
    fencePool.waitOnCpu(fenceHandle);
    queues.deallocate(list, fenceHandle);
    fencePool.free(fenceHandle);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void VulkanImguiRenderer::initPass()
{
    VkAttachmentDescription attachment = {};
    attachment.format = getVkFormat(m_display.resolvedFormat());
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachment = {};
    colorAttachment.attachment = 0;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachment;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 0;
    info.pDependencies = nullptr;
    VK_OK(vkCreateRenderPass(m_device.vkDevice(), &info, nullptr, &m_vkRenderPass));
}

void VulkanImguiRenderer::initDescriptorPool()
{
    const VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 64 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 32;
    poolInfo.poolSizeCount = (int)(sizeof(poolSizes)/sizeof(poolSizes[0]));
    poolInfo.pPoolSizes = poolSizes;

    m_vkDescriptorPool = {};
    VK_OK(vkCreateDescriptorPool(m_device.vkDevice(), &poolInfo, nullptr, &m_vkDescriptorPool));
}

void VulkanImguiRenderer::setupSwapChain()
{
    const auto& displayInfo = m_display.config();
    if (m_cachedHeight == displayInfo.height && m_cachedWidth == displayInfo.width && m_cachedSwapVersion == m_display.version())
        return;

    if (m_vkFrameBuffer)
        vkDestroyFramebuffer(m_device.vkDevice(), m_vkFrameBuffer, nullptr) ;

    m_cachedHeight = displayInfo.height;
    m_cachedWidth = displayInfo.width;
    m_cachedSwapVersion = m_display.version();
    
    VulkanResources& resources = m_device.resources();
    VulkanResource& textureResource = resources.unsafeGetResource(m_display.texture());
    CPY_ASSERT(textureResource.isTexture())
    VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO , nullptr };
    info.renderPass = m_vkRenderPass;
    info.attachmentCount = 1;
    info.pAttachments = &textureResource.textureData.vkSrvView;
    info.width = textureResource.textureData.width;
    info.height = textureResource.textureData.height;
    info.layers = 1;
    VK_OK(vkCreateFramebuffer(m_device.vkDevice(), &info, nullptr, &m_vkFrameBuffer));
}

void VulkanImguiRenderer::newFrame()
{
    BaseImguiRenderer::newFrame();
    activate();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void VulkanImguiRenderer::render()
{
    setupSwapChain();

    ImGui::Render();

    VulkanQueues& queues = m_device.queues();
    VulkanFencePool& fencePool = m_device.fencePool();
    VulkanFenceHandle fenceHandle = fencePool.allocate();

    VkQueue queue = queues.cmdQueue(WorkType::Graphics);
    VulkanList list;
    queues.allocate(WorkType::Graphics, list);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(list.list, &beginInfo);

    BarrierRequest barrierRequest = { m_display.texture(), ResourceGpuState::Rtv };
    inlineApplyBarriers(m_device, &barrierRequest, 1, list.list);

    VkRenderPassBeginInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO , nullptr};
    info.renderPass = m_vkRenderPass;
    info.framebuffer = m_vkFrameBuffer;
    info.renderArea.extent.width = m_cachedWidth;
    info.renderArea.extent.height = m_cachedHeight;
    info.clearValueCount = 0;
    vkCmdBeginRenderPass(list.list, &info, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), list.list);

    vkCmdEndRenderPass(list.list);

    vkEndCommandBuffer(list.list);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &list.list;

    VK_OK(vkQueueSubmit(queue, 1u, &submitInfo, fencePool.get(fenceHandle)));
    queues.deallocate(list, fenceHandle);
    fencePool.free(fenceHandle);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

ImTextureID VulkanImguiRenderer::registerTexture(Texture texture)
{
    return {};
}

void VulkanImguiRenderer::unregisterTexture(Texture texture)
{
}

bool VulkanImguiRenderer::isTextureRegistered(Texture texture) const
{
    return false;
}

}
}

