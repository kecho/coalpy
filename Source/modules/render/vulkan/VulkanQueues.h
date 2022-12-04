#pragma once

#include "Config.h"
#include <vector>
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
    struct AllocatorRecord
    {
        uint64_t fenceValue = 0;
    };

    struct QueueContainer
    {
        VkQueue queue = {};
        VulkanFence* fence = nullptr;
        std::vector<AllocatorRecord> allocatorPool;
        std::vector<VkCommandBuffer> listPool;
        VulkanMemoryPools memPools;
    };

    QueueContainer m_containers[(int)WorkType::Count];
    VulkanDevice& m_device;
};


}
}
