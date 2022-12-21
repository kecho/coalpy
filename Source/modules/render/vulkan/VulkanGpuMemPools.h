#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <coalpy.render/Resources.h>
#include "VulkanFencePool.h"
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
    enum
    {
        DefaultUploadPoolSize = 5 * 1024 * 1024 ///5 mb of initial size
    };
    VulkanGpuUploadPool(VulkanDevice& device, VulkanFencePool& fencePool, uint64_t initialPoolSize = DefaultUploadPoolSize);
    ~VulkanGpuUploadPool();
    void beginUsage(VulkanFenceHandle handle);
    void endUsage();

    VulkanGpuMemoryBlock allocUploadBlock(size_t sizeBytes);

private:
    class VulkanGpuUploadPoolImpl* m_impl;
};

class VulkanGpuDescriptorSetPool
{
public:
    VulkanGpuDescriptorSetPool(VulkanDevice& device, VulkanFencePool& fencePool);
    ~VulkanGpuDescriptorSetPool();
    void beginUsage(VulkanFenceHandle handle);
    void endUsage();
    VkDescriptorSet allocUploadBlock(VkDescriptorSetLayout layout);

private:
    VkDescriptorPool newPool() const;

    VulkanDevice& m_device;
    VulkanFencePool& m_fencePool;

    struct PoolState
    {
        VkDescriptorPool pool;
        VulkanFenceHandle fenceVal;
    };

    VulkanFenceHandle m_currentFence;
    std::queue<int> m_freePools;
    std::queue<int> m_livePools;
    std::vector<PoolState> m_pools;
    int m_activePool;
};

}
}
