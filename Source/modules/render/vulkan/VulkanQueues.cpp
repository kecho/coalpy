#include <Config.h>
#if ENABLE_VULKAN

#include "VulkanQueues.h"
#include "VulkanDevice.h"
#include "VulkanDevice.h"
#include <coalpy.core/Assert.h>
#include "VulkanGpuMemPools.h"
#include <iostream>

namespace coalpy
{
namespace render
{

VulkanQueues::VulkanQueues(VulkanDevice& device, VulkanFencePool& fencePool, VulkanEventPool& eventPool)
: m_device(device), m_fencePool(fencePool), m_eventPool(eventPool)
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

void VulkanQueues::releaseResources()
{
    for (int workType = 0; workType < (int)WorkType::Count; ++workType)
    {
        QueueContainer& container =  m_containers[workType];
        if (container.memPools.uploadPool)
            delete container.memPools.uploadPool;
        if (container.memPools.descriptors)
            delete container.memPools.descriptors;
        container.memPools = {};
    }
}

VulkanQueues::~VulkanQueues()
{
    releaseResources();

    for (int workType = 0; workType < (int)WorkType::Count; ++workType)
    {
        QueueContainer& container =  m_containers[workType];
        waitForAllWorkOnCpu((WorkType)workType);
        syncFences((WorkType)workType);
        garbageCollectCmdBuffers((WorkType)workType);
    }

    vkDestroyCommandPool(m_device.vkDevice(), m_cmdPool, nullptr);
}

VulkanFenceHandle VulkanQueues::newFence()
{
    return m_fencePool.allocate(); 
}

void VulkanQueues::syncFences(WorkType workType)
{
    QueueContainer& container = m_containers[(int)workType];
    for (int i = 0; i < container.liveAllocationsCount; ++i)
        m_fencePool.updateState(container.liveAllocations[(i + container.liveAllocationsBegin) % MaxLiveAllocations].fenceValue);
}

void VulkanQueues::waitForAllWorkOnCpu(WorkType workType)
{
    QueueContainer& container = m_containers[(int)workType];
    for (int i = 0; i < container.liveAllocationsCount; ++i)
        m_fencePool.waitOnCpu(container.liveAllocations[(i + container.liveAllocationsBegin) % MaxLiveAllocations].fenceValue);
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
    while (container.liveAllocationsCount > 0)
    {
        auto& allocation = container.frontAllocation();
        if (m_fencePool.isSignaled(allocation.fenceValue))
        {
            freeCmdBuffers.push_back(allocation.list);
            m_fencePool.free(allocation.fenceValue);
            container.popAllocation();
        }
        break;
    }

    if (!freeCmdBuffers.empty())
        vkFreeCommandBuffers(m_device.vkDevice(), m_cmdPool, freeCmdBuffers.size(), freeCmdBuffers.data());
}

void VulkanQueues::deallocate(VulkanList& list, VulkanFenceHandle fenceValue)
{
    CPY_ASSERT((int)list.workType >= 0 && (int)list.workType < (int)WorkType::Count);
    auto& q = m_containers[(int)list.workType];
    m_fencePool.addRef(fenceValue);
    LiveAllocation alloc = { fenceValue, list.list };
    q.pushAllocation(alloc);
    list = {};
}

}
}

#endif // ENABLE_VULKAN