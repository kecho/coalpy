#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <coalpy.render/Resources.h>
#include <queue>

namespace coalpy
{
namespace render
{

class VulkanDevice;
class VulkanFence;

struct VulkanGpuMemoryBlock
{
    size_t uploadSize = 0ull;
    void* mappedBuffer = nullptr;
    uint64_t offset = 0ull;
    Buffer buffer;
};

class VulkanGpuUploadPool
{
public:
    VulkanGpuUploadPool(VulkanDevice& device, VkQueue queue, uint64_t initialPoolSize);
    ~VulkanGpuUploadPool();
    void beginUsage();
    void endUsage();

    VulkanGpuMemoryBlock allocUploadBlock(size_t sizeBytes);

private:
    class VulkanGpuUploadPoolImpl* m_impl;
};

class VulkanGpuDescriptorSetPool
{
public:
    VulkanGpuDescriptorSetPool(VulkanDevice& device, VkQueue queue);
    ~VulkanGpuDescriptorSetPool();
    void beginUsage();
    void endUsage();
    VkDescriptorSet allocUploadBlock(VkDescriptorSetLayout layout);

private:
    VkDescriptorPool newPool() const;

    VulkanDevice& m_device;
    VulkanFence& m_fence;

    struct PoolState
    {
        VkDescriptorPool pool;
        uint64_t fenceVal;
    };

    std::queue<int> m_freePools;
    std::queue<int> m_livePools;
    std::vector<PoolState> m_pools;
    int m_activePool;
    
    uint64_t m_nextFenceVal;
};

}
}
