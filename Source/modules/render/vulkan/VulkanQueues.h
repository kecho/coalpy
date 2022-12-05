#pragma once

#include "Config.h"
#include <vector>
#include <queue>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace coalpy
{
namespace render
{

class VulkanDevice;
class VulkanFence;

enum class WorkType
{
    Graphics,
    Compute,
    Copy,
    Count
};

struct VulkanList
{
    WorkType workType = WorkType::Graphics;
    VkCommandBuffer list;
};

struct VulkanMemoryPools
{
    /* TODO: implement here ring buffer pools */
};

class VulkanQueues
{
public:
    VulkanQueues(VulkanDevice& device);
    ~VulkanQueues();

    VkQueue& cmdQueue(WorkType type) { return (m_containers[(int)type].queue); }
    VulkanMemoryPools& memPools(WorkType type) { return m_containers[(int)type].memPools; }
    VulkanFence& getFence(WorkType type) { return *(m_containers[(int)type].fence); }
    
    void allocate(WorkType workType, VulkanList& outList);
    uint64_t currentFenceValue(WorkType workType);
    uint64_t signalFence(WorkType workType);
    void deallocate(VulkanList& list, uint64_t fenceValue);

private:
    void garbageCollectCmdBuffers(WorkType workType);

    struct LiveAllocation
    {
        uint64_t fenceValue = 0;
        VkCommandBuffer list;
    };

    struct QueueContainer
    {
        VkQueue queue = {};
        VulkanFence* fence = nullptr;
        std::queue<LiveAllocation> liveAllocations;
        VulkanMemoryPools memPools;
    };

    QueueContainer m_containers[(int)WorkType::Count];
    
    VkCommandPool m_cmdPool;
    VulkanDevice& m_device;
};


}
}
