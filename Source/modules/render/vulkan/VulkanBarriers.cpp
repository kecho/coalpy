#include "VulkanBarriers.h"
#include "VulkanDevice.h"
#include "VulkanEventPool.h"
#include "VulkanResources.h"

namespace coalpy
{
namespace render
{

EventState createSrcBarrierEvent(
    VulkanDevice& device,
    VulkanEventPool& eventPool,
    const std::vector<ResourceBarrier>& barriers,
    VkCommandBuffer cmdBuffer)
{
    CommandLocation srcLocation;
    EventState eventState;
    bool allocateEvent = false;
    bool mustReset = false;
    for (auto& b : barriers)
    {
        if (b.type != BarrierType::Begin)
            continue;

        if (!allocateEvent)
        {
            srcLocation = b.srcCmdLocation;
            allocateEvent = true;
        }

        CPY_ASSERT(srcLocation == b.srcCmdLocation);
        eventState.flags |= getVkStage(b.prevState);
    }

    if (allocateEvent)
    {
        CPY_ASSERT(!eventState.eventHandle.valid());
        bool isNew = false;
        eventState.eventHandle = eventPool.allocate(srcLocation, eventState.flags, isNew);
        mustReset = !isNew;
    }

    if (mustReset)
    {
        VkEvent event = eventPool.getEvent(eventState.eventHandle);
        vkCmdResetEvent(cmdBuffer, event, eventState.flags);
    }

    return eventState;
}

void applyBarriers(
    VulkanDevice& device,
    const EventState& srcEvent,
    VulkanEventPool& eventPool,
    const std::vector<ResourceBarrier>& barriers,
    VkCommandBuffer cmdBuffer)
{
    if (barriers.empty())
        return;

    VkPipelineStageFlags immSrcFlags = 0;
    VkPipelineStageFlags immDstFlags = 0;
    std::vector<VkBufferMemoryBarrier> immBufferBarriers;
    std::vector<VkImageMemoryBarrier> immImageBarriers;

    VkBufferMemoryBarrier buffBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr };
    VkImageMemoryBarrier imgBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr };

    VulkanResources& resources = device.resources();

    struct DstEventState : public EventState
    {
        VkPipelineStageFlags dstFlags = 0;
        std::vector<VkImageMemoryBarrier> imageBarriers;
        std::vector<VkBufferMemoryBarrier> bufferBarriers;
    };

    CommandLocation srcLocation;
    std::unordered_map<CommandLocation, DstEventState, CommandLocationHasher> dstEvents;

    for (const auto& b : barriers)
    {
        if (b.isUav)
            continue;

        DstEventState* dstEventPtr = nullptr;
        if (b.type == BarrierType::Begin)
        {
            CPY_ASSERT(srcEvent.eventHandle.valid());
        }
        else if (b.type == BarrierType::End)
        {
            auto it = dstEvents.find(b.srcCmdLocation);
            if (it == dstEvents.end())
            {
                VulkanEventHandle eventHandle = eventPool.find(b.srcCmdLocation);
                CPY_ASSERT(eventHandle.valid());
                DstEventState dstEvent;
                dstEvent.eventHandle = eventHandle;
                dstEventPtr = &dstEvents.insert(std::pair<CommandLocation, DstEventState>(b.srcCmdLocation, dstEvent)).first->second;
            }
            else
            {
                dstEventPtr = &it->second;
            }
            dstEventPtr->flags |= getVkStage(b.prevState);
            dstEventPtr->dstFlags |= getVkStage(b.postState);
        }
        else
        {
            immSrcFlags |= getVkStage(b.prevState);
            immDstFlags |= getVkStage(b.postState);
        }

        if (b.type == BarrierType::Begin)
            continue;

        std::vector<VkBufferMemoryBarrier>& bufferBarriers = b.type == BarrierType::Immediate ? immBufferBarriers : dstEventPtr->bufferBarriers;
        std::vector<VkImageMemoryBarrier>& imgBarriers = b.type == BarrierType::Immediate ? immImageBarriers : dstEventPtr->imageBarriers;
        
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

    if (srcEvent.eventHandle.valid())
    {
        VkEvent event = eventPool.getEvent(srcEvent.eventHandle);
        vkCmdSetEvent(cmdBuffer, event, srcEvent.flags);
    }

    if (immSrcFlags != 0 || immDstFlags != 0)
        vkCmdPipelineBarrier(cmdBuffer, immSrcFlags, immDstFlags, 0, 0, nullptr,
        immBufferBarriers.size(), immBufferBarriers.data(), immImageBarriers.size(), immImageBarriers.data());

    for (auto pairVal : dstEvents)
    {
        DstEventState& dstEvent = pairVal.second;
        VkEvent event = eventPool.getEvent(dstEvent.eventHandle);
        VkPipelineStageFlags srcEventFlags = eventPool.getFlags(dstEvent.eventHandle);
        vkCmdWaitEvents(
            cmdBuffer, 1, &event, srcEventFlags, dstEvent.dstFlags,
            0, nullptr,
            dstEvent.bufferBarriers.size(), dstEvent.bufferBarriers.data(),
            dstEvent.imageBarriers.size(), dstEvent.imageBarriers.data());
    }
}

}
}