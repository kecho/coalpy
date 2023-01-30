#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "VulkanWorkBundle.h"

namespace coalpy
{
namespace render
{

class VulkanDevice;
class VulkanEventPool;

struct EventState
{
    VulkanEventHandle eventHandle;
    VkPipelineStageFlags flags = 0;
};

struct BarrierRequest
{
    ResourceHandle resource;
    ResourceGpuState state;
};

inline VkPipelineStageFlagBits getVkStage(ResourceGpuState state)
{
    switch (state)
    {
        case ResourceGpuState::Default:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case ResourceGpuState::IndirectArgs:
            return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
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
        case ResourceGpuState::Uninitialized:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    CPY_ASSERT_FMT(false, "vulkan's state used is not handled in coalpy's rendering", state);
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
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case ResourceGpuState::Present:
            return VK_ACCESS_MEMORY_WRITE_BIT;
        case ResourceGpuState::Uninitialized:
            return VK_ACCESS_NONE;
    }
    CPY_ASSERT_FMT(false, "vulkan's state used is not handled in coalpy's rendering", state);
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
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ResourceGpuState::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case ResourceGpuState::Uninitialized:
            return VK_IMAGE_LAYOUT_PREINITIALIZED;
    }
    CPY_ASSERT_FMT(false, "vulkan's state used is not handled in coalpy's rendering", state);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

EventState createSrcBarrierEvent(
    VulkanDevice& device,
    VulkanEventPool& eventPool,
    const ResourceBarrier* barriers,
    int barriersCount,
    VkCommandBuffer cmdBuffer);

void applyBarriers(
    VulkanDevice& device,
    const EventState& srcEvent,
    VulkanEventPool& eventPool,
    const ResourceBarrier* barriers,
    int barriersCount,
    VkCommandBuffer cmdBuffer);

void inlineApplyBarriers(
    VulkanDevice& device,
    const BarrierRequest* requests,
    int requestsCount,
    VkCommandBuffer cmdBuffer);

}
}
