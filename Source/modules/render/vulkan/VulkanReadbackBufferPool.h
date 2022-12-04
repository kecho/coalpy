#pragma once

#include <coalpy.render/Resources.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>


namespace coalpy
{
namespace render
{

class VulkanDevice;

struct VulkanReadbackMemBlock
{
    size_t size = 0ull;
    size_t offset = 0ull;
    void* mappedMemory = nullptr;
    Buffer buffer = {};
    short allocationId = 0;
    short heapIndex = 0;
    bool valid() const { return buffer.valid(); }
};

class VulkanReadbackBufferPool
{
public:
    VulkanReadbackBufferPool(VulkanDevice& device);
    ~VulkanReadbackBufferPool();
    
    VulkanReadbackMemBlock allocate(size_t size);
    void free(const VulkanReadbackMemBlock& block);

private:
    bool createNewHeap(size_t size);
    
    VulkanDevice& m_device;
    
    struct HeapState
    {
        Buffer buffer;
        std::vector<VulkanReadbackMemBlock> freeBlocks;
        size_t largestSize = 0ull;
    };

    std::vector<HeapState> m_heaps;
    size_t m_nextHeapSize = 0;
    short m_nextAllocId = 0;
};

}
}
