#include "VulkanQueues.h"
#include "VulkanFence.h"
#include "VulkanDevice.h"
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

VulkanQueues::VulkanQueues(VulkanDevice& device)
: m_device(device)
{
    for (int queueIt = 0u; queueIt < (int)WorkType::Count; ++queueIt)
    {
        QueueContainer& qcontainer = m_containers[queueIt];
        vkGetDeviceQueue(m_device.vkDevice(), m_device.graphicsFamilyQueueIndex(), (uint32_t)queueIt, &qcontainer.queue);
        qcontainer.fence = new VulkanFence(device, qcontainer.queue);
    }

    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = m_device.graphicsFamilyQueueIndex();
    VK_OK(vkCreateCommandPool(m_device.vkDevice(), &poolCreateInfo, nullptr, &m_cmdPool));
}

VulkanQueues::~VulkanQueues()
{
    for (auto& container : m_containers)
    {
        while (!container.liveAllocations.empty())
        {
            container.fence->waitOnCpu(container.liveAllocations.front().fenceValue);
            container.liveAllocations.pop();
        }

        
        delete container.fence;
    }

    vkDestroyCommandPool(m_device.vkDevice(), m_cmdPool, nullptr);
}

void VulkanQueues::allocate(WorkType workType, VulkanList& outList)
{
    garbageCollectCmdBuffers(workType);
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
    allocInfo.commandPool = m_cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    outList.workType = workType;
    VK_OK(vkAllocateCommandBuffers(m_device.vkDevice(), &allocInfo, &outList.list));
}

void VulkanQueues::garbageCollectCmdBuffers(WorkType workType)
{
    auto& container = m_containers[(int)workType];
    uint64_t currentFenceValue = container.fence->completedValue();
    std::vector<VkCommandBuffer> freeCmdBuffers;
    while (!container.liveAllocations.empty())
    {
        auto& allocation = container.liveAllocations.front();
        if (allocation.fenceValue <= currentFenceValue)
        {
            freeCmdBuffers.push_back(allocation.list);
            container.liveAllocations.pop();
        }

        break;
    }

    vkFreeCommandBuffers(m_device.vkDevice(), m_cmdPool, freeCmdBuffers.size(), freeCmdBuffers.data());
}

uint64_t VulkanQueues::currentFenceValue(WorkType type)
{
    CPY_ASSERT((int)type >= 0 && (int)type < (int)WorkType::Count);
    auto& container = m_containers[(int)type];
    return container.fence->completedValue();
}

uint64_t VulkanQueues::signalFence(WorkType workType)
{
    CPY_ASSERT((int)workType >= 0 && (int)workType < (int)WorkType::Count);
    auto& q = m_containers[(int)workType];
    return q.fence->signal();
}

void VulkanQueues::deallocate(VulkanList& list, uint64_t fenceValue)
{
    CPY_ASSERT((int)list.workType >= 0 && (int)list.workType < (int)WorkType::Count);
    auto& q = m_containers[(int)list.workType];
    q.liveAllocations.push(LiveAllocation { fenceValue, list.list });
    list = {};
}

}
}
