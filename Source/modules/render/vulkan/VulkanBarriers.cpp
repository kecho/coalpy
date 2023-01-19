#include "VulkanBarriers.h"
#include "VulkanDevice.h"
#include "VulkanEventPool.h"
#include "VulkanResources.h"
#include "WorkBundleDb.h"
#include <vector>

namespace coalpy
{
namespace render
{

EventState createSrcBarrierEvent(
    VulkanDevice& device,
    VulkanEventPool& eventPool,
    const ResourceBarrier* barriers,
    int barriersCount,
    VkCommandBuffer cmdBuffer)
{
    CommandLocation srcLocation;
    EventState eventState;
    bool allocateEvent = false;
    bool mustReset = false;
    for (int i = 0; i < barriersCount; ++i)
    {
        const ResourceBarrier& b = barriers[i];
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
    const ResourceBarrier* barriers,
    int barriersCount,
    VkCommandBuffer cmdBuffer)
{
    if (barriersCount == 0)
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

    for (int i = 0; i < barriersCount; ++i)
    {
        const auto& b = barriers[i];
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
            imgBarriers.push_back(newBarrier);
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

void inlineApplyBarriers(
    VulkanDevice& device,
    const BarrierRequest* requests,
    int requestsCount,
    VkCommandBuffer cmdBuffer)
{
    WorkBundleDb& workDb = device.workDb();
    WorkResourceInfos& resourceInfos = workDb.resourceInfos();
    
    std::vector<ResourceBarrier> barriers;
    barriers.reserve(requestsCount);

    //initialize resource barriers
    for (int i = 0; i < requestsCount; ++i)
    {
        const BarrierRequest& request = requests[i];
        auto it = resourceInfos.find(request.resource);
        CPY_ASSERT(it != resourceInfos.end());
        if (it == resourceInfos.end())
            continue;
        
        WorkResourceInfo& resourceInfo = it->second;
        ResourceGpuState prevState = resourceInfo.gpuState;
        if (prevState == request.state)
            continue;

        //update the resource info
        resourceInfo.gpuState = request.state;
        barriers.emplace_back();
        ResourceBarrier& newBarrier = barriers.back();
        newBarrier.resource = request.resource;
        newBarrier.prevState = prevState;
        newBarrier.postState = request.state;
        newBarrier.type = BarrierType::Immediate;
    }
    
    EventState blankState;
    applyBarriers(device, blankState, device.eventPool(), barriers.data(), (int)barriers.size(), cmdBuffer);
}

}
}
