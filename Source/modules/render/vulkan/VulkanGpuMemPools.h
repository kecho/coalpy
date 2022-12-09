#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <coalpy.render/Resources.h>

namespace coalpy
{
namespace render
{

class VulkanDevice;

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

}
}
