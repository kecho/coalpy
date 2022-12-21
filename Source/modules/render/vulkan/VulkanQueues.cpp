#include "VulkanQueues.h"
#include "VulkanDevice.h"
#include "VulkanDevice.h"
#include <coalpy.core/Assert.h>
#include "VulkanGpuMemPools.h"

namespace coalpy
{
namespace render
{

VulkanQueues::VulkanQueues(VulkanDevice& device, VulkanFencePool& fencePool)
: m_device(device), m_fencePool(fencePool)
{
    for (int queueIt = 0u; queueIt < (int)WorkType::Count; ++queueIt)
    {
        QueueContainer& qcontainer = m_containers[queueIt];
        vkGetDeviceQueue(m_device.vkDevice(), m_device.graphicsFamilyQueueIndex(), (uint32_t)queueIt, &qcontainer.queue);
        qcontainer.memPools.uploadPool = new VulkanGpuUploadPool(device, device.fencePool());
        qcontainer.memPools.descriptors = new VulkanGpuDescriptorSetPool(device, device.fencePool());
    }

    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = m_device.graphicsFamilyQueueIndex();
    VK_OK(vkCreateCommandPool(m_device.vkDevice(), &poolCreateInfo, nullptr, &m_cmdPool));
}

VulkanQueues::~VulkanQueues()
{
    for (auto& container : m_containers)
    for (int workType = 0; workType < (int)WorkType::Count; ++workType)
    {
        QueueContainer& container =  m_containers[workType];
        waitForAllWorkOnCpu((WorkType)workType);
        syncFences((WorkType)workType);
        cleanSignaledFences((WorkType)workType);
        delete container.memPools.uploadPool;
        delete container.memPools.descriptors;
    }

    vkDestroyCommandPool(m_device.vkDevice(), m_cmdPool, nullptr);
}

VulkanFenceHandle VulkanQueues::fence(WorkType workType)
{
    QueueContainer& container = m_containers[(int)workType];
    CPY_ASSERT(container.activeFenceCount < MaxFences);
    if (container.activeFenceCount >= MaxFences)
    {
        m_fencePool.waitOnCpu(container.frontFence());
        m_fencePool.free(container.frontFence());
        container.popFence();
    }
    VulkanFenceHandle handle = m_fencePool.allocate();
    container.pushFence(handle);
    return handle; 
}

void VulkanQueues::syncFences(WorkType workType)
{
    QueueContainer& container = m_containers[(int)workType];
    for (int i = 0; i < container.activeFenceCount; ++i)
        m_fencePool.updateState(container.activeFences[i % MaxFences]);
}

void VulkanQueues::cleanSignaledFences(WorkType workType)
{
    QueueContainer& container = m_containers[(int)workType];
    while (container.activeFenceCount != 0)
    {
        if (!m_fencePool.isSignaled(container.frontFence()))
            break;

        m_fencePool.free(container.frontFence());
        container.popFence();
    }
}

void VulkanQueues::waitForAllWorkOnCpu(WorkType workType)
{
    QueueContainer& container = m_containers[(int)workType];
    for (int i = 0; i < container.activeFenceCount; ++i)
        m_fencePool.waitOnCpu(container.activeFences[i % MaxFences]);
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
    std::vector<VkCommandBuffer> freeCmdBuffers;
    while (!container.liveAllocations.empty())
    {
        auto& allocation = container.liveAllocations.front();
        if (m_fencePool.isSignaled(allocation.fenceValue))
        {
            freeCmdBuffers.push_back(allocation.list);
            container.liveAllocations.pop();
        }

        break;
    }

    vkFreeCommandBuffers(m_device.vkDevice(), m_cmdPool, freeCmdBuffers.size(), freeCmdBuffers.data());
}

void VulkanQueues::deallocate(VulkanList& list, VulkanFenceHandle fenceValue)
{
    CPY_ASSERT((int)list.workType >= 0 && (int)list.workType < (int)WorkType::Count);
    auto& q = m_containers[(int)list.workType];
    q.liveAllocations.push(LiveAllocation { fenceValue, list.list });
    list = {};
}

}
}
