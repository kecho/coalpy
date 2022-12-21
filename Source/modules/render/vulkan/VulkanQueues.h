#pragma once

#include "Config.h"
#include "VulkanFencePool.h"
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
    VulkanQueues(VulkanDevice& device, VulkanFencePool& fencePool);
    ~VulkanQueues();

    VkQueue& cmdQueue(WorkType type) { return (m_containers[(int)type].queue); }
    VulkanMemoryPools& memPools(WorkType type) { return m_containers[(int)type].memPools; }
    
    VulkanFenceHandle fence(WorkType workType);
    void cleanSignaledFences(WorkType workType);
    void syncFences(WorkType workType);
    void waitForAllWorkOnCpu(WorkType workType);
    void allocate(WorkType workType, VulkanList& outList);
    uint64_t currentFenceValue(WorkType workType);
    void deallocate(VulkanList& list, VulkanFenceHandle fenceValue);

    VulkanFencePool& fencePool() { return m_fencePool; }

private:
    void garbageCollectCmdBuffers(WorkType workType);

    struct LiveAllocation
    {
        VulkanFenceHandle fenceValue;
        VkCommandBuffer list;
    };

    enum : int { MaxFences = 512 };

    struct QueueContainer
    {
        VkQueue queue = {};
        VulkanFenceHandle fence;
        std::queue<LiveAllocation> liveAllocations;
        VulkanFenceHandle activeFences[MaxFences];
        int activeFenceBegin = 0;
        int activeFenceCount = 0;
        VulkanMemoryPools memPools;

        VulkanFenceHandle frontFence() const
        {
            return activeFences[activeFenceBegin];
        }

        void popFence()
        {
            activeFenceBegin = (activeFenceBegin + 1) % MaxFences;
            --activeFenceCount;
        }

        void pushFence(VulkanFenceHandle handle)
        {
            activeFences[(activeFenceBegin + activeFenceCount++) % MaxFences] = handle;
        }
    };

    QueueContainer m_containers[(int)WorkType::Count];
    
    VkCommandPool m_cmdPool;
    VulkanFencePool& m_fencePool;
    VulkanDevice& m_device;
};


}
}
