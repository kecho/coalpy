#pragma once

#include "Config.h"
#include "VulkanFencePool.h"
#include "VulkanEventPool.h"
#include <vector>
#include <queue>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace coalpy
{
namespace render
{

class VulkanDevice;
class VulkanGpuUploadPool;
class VulkanGpuDescriptorSetPool;

enum class WorkType
{
    Graphics,
    //Compute,
    //Copy,
    Count
};

struct VulkanList
{
    WorkType workType = WorkType::Graphics;
    VkCommandBuffer list;
};

struct VulkanMemoryPools
{
    VulkanGpuUploadPool* uploadPool;
    VulkanGpuDescriptorSetPool* descriptors;
};

class VulkanQueues
{
public:
    VulkanQueues(VulkanDevice& device, VulkanFencePool& fencePool, VulkanEventPool& eventPool);
    ~VulkanQueues();
    void releaseResources();

    VkQueue& cmdQueue(WorkType type) { return (m_containers[(int)type].queue); }
    VulkanMemoryPools& memPools(WorkType type) { return m_containers[(int)type].memPools; }
    
    VulkanFenceHandle newFence();
    void syncFences(WorkType workType);
    void waitForAllWorkOnCpu(WorkType workType);
    void allocate(WorkType workType, VulkanList& outList);
    uint64_t currentFenceValue(WorkType workType);
    void deallocate(VulkanList& list, VulkanFenceHandle fenceValue);

    VulkanFencePool& fencePool() { return m_fencePool; }
    VulkanEventPool& eventPool() { return m_eventPool; }

private:
    void garbageCollectCmdBuffers(WorkType workType);

    struct LiveAllocation
    {
        VulkanFenceHandle fenceValue;
        VkCommandBuffer list;
    };

    enum : int { MaxLiveAllocations = 512 };

    struct QueueContainer
    {
        VkQueue queue = {};
        LiveAllocation liveAllocations[MaxLiveAllocations];
        int liveAllocationsBegin = 0;
        int liveAllocationsCount = 0;
        VulkanMemoryPools memPools;

        LiveAllocation& frontAllocation()
        {
            return liveAllocations[liveAllocationsBegin];
        }

        void popAllocation()
        {
            liveAllocationsBegin = (liveAllocationsBegin + 1) % MaxLiveAllocations;
            --liveAllocationsCount;
        }

        void pushAllocation(LiveAllocation& liveAllocation)
        {
            liveAllocations[(liveAllocationsBegin + liveAllocationsCount++) % MaxLiveAllocations] = liveAllocation;
        }
    };

    QueueContainer m_containers[(int)WorkType::Count];
    
    VkCommandPool m_cmdPool;
    VulkanFencePool& m_fencePool;
    VulkanEventPool& m_eventPool;
    VulkanDevice& m_device;
};


}
}
