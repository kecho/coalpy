#include "VulkanWorkBundle.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include <coalpy.core/Assert.h>
#include <vector>

namespace coalpy
{
namespace render
{

namespace
{

inline VkPipelineStageFlagBits getVkStage(ResourceGpuState state)
{
    switch (state)
    {
        case ResourceGpuState::Default:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case ResourceGpuState::IndirectArgs:
        case ResourceGpuState::Uav:
        case ResourceGpuState::Srv:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case ResourceGpuState::CopyDst:
        case ResourceGpuState::CopySrc:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case ResourceGpuState::Cbv:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case ResourceGpuState::Rtv:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case ResourceGpuState::Present:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    CPY_ASSERT_FMT(false, "D3d12 state used is not handled in coalpy's rendering", state);
    return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
}

inline VkAccessFlags getVkAccessMask(ResourceGpuState state)
{
    switch (state)
    {
        case ResourceGpuState::Default:
            return VK_ACCESS_NONE;
        case ResourceGpuState::IndirectArgs:
            return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case ResourceGpuState::Uav:
            return VK_ACCESS_SHADER_WRITE_BIT;
        case ResourceGpuState::Srv:
            return VK_ACCESS_SHADER_READ_BIT;
        case ResourceGpuState::CopyDst:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case ResourceGpuState::CopySrc:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case ResourceGpuState::Cbv:
            return VK_ACCESS_UNIFORM_READ_BIT;
        case ResourceGpuState::Rtv:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case ResourceGpuState::Present:
            return VK_ACCESS_MEMORY_WRITE_BIT;
    }
    CPY_ASSERT_FMT(false, "D3d12 state used is not handled in coalpy's rendering", state);
    return VK_ACCESS_NONE;
}

inline VkImageLayout getVkImageLayout(ResourceGpuState state)
{
    switch (state)
    {
        case ResourceGpuState::Default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case ResourceGpuState::IndirectArgs:
        case ResourceGpuState::Uav:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceGpuState::Srv:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ResourceGpuState::CopyDst:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ResourceGpuState::CopySrc:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ResourceGpuState::Cbv:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case ResourceGpuState::Rtv:
        case ResourceGpuState::Present:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    CPY_ASSERT_FMT(false, "D3d12 state used is not handled in coalpy's rendering", state);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

void applyBarriers(VulkanDevice& device, const std::vector<ResourceBarrier>& barriers, VkCommandBuffer cmdBuffer)
{
    if (barriers.empty())
        return;

    VkPipelineStageFlags stageBefore[(int)BarrierType::Count] = {};
    VkPipelineStageFlags stageAfter[(int)BarrierType::Count] = {};
    std::vector<VkBufferMemoryBarrier> splitBufferBarriers;
    std::vector<VkBufferMemoryBarrier> immBufferBarriers;
    std::vector<VkImageMemoryBarrier> splitImageBarriers;
    std::vector<VkImageMemoryBarrier> immImageBarriers;

    VkBufferMemoryBarrier buffBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr };
    VkImageMemoryBarrier imgBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr };

    VulkanResources& resources = device.resources();

    for (const auto& b : barriers)
    {
        if (b.isUav)
            continue;

        stageBefore[(int)b.type] |= getVkStage(b.prevState);
        stageAfter[(int)b.type] |= getVkStage(b.postState);

        if (b.type == BarrierType::Begin)
            continue;
    
        std::vector<VkBufferMemoryBarrier>& bufferBarriers = b.type == BarrierType::Immediate ? immBufferBarriers : splitBufferBarriers;
        std::vector<VkImageMemoryBarrier>& imgBarriers = b.type == BarrierType::Immediate ? immImageBarriers : splitImageBarriers;
        
        VulkanResource& resource = resources.unsafeGetResource(b.resource);
        VkAccessFlags srcAccessMask = getVkAccessMask(b.prevState);
        VkAccessFlags dstAccessMask = getVkAccessMask(b.postState);
        if (resource.isBuffer())
        {
            auto newBarrier = buffBarrier;
            newBarrier.srcAccessMask = srcAccessMask;
            newBarrier.dstAccessMask = dstAccessMask;
            newBarrier.srcQueueFamilyIndex = device.graphicsFamilyQueueIndex();
            newBarrier.dstQueueFamilyIndex = device.graphicsFamilyQueueIndex();
            newBarrier.buffer = resource.bufferData.vkBuffer;
            newBarrier.offset = 0ull;
            newBarrier.size = resource.bufferData.size;
            bufferBarriers.push_back(newBarrier);
        }
        else
        {
            auto newBarrier = imgBarrier;
            newBarrier.srcAccessMask = srcAccessMask;
            newBarrier.dstAccessMask = dstAccessMask;
            newBarrier.oldLayout = getVkImageLayout(b.prevState);
            newBarrier.newLayout = getVkImageLayout(b.postState);
            newBarrier.srcQueueFamilyIndex = device.graphicsFamilyQueueIndex();
            newBarrier.dstQueueFamilyIndex = device.graphicsFamilyQueueIndex();
            newBarrier.image = resource.textureData.vkImage;
            newBarrier.subresourceRange = resource.textureData.subresourceRange;
        }
    }
}

}

bool VulkanWorkBundle::load(const WorkBundle& workBundle)
{
    return false;
}

uint64_t VulkanWorkBundle::execute(CommandList** commandLists, int commandListsCount)
{
    return {};
}

}
}
