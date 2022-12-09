#include "VulkanGpuMemPools.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanFence.h"
#include "VulkanUtils.h"
#include "TGpuResourcePool.h"
#include <coalpy.core/Assert.h>
#include <algorithm>

namespace coalpy
{
namespace render
{

struct VulkanUploadDesc
{
    uint64_t alignment = 0ull;
    uint64_t requestBytes = 0ull;
};

struct VulkanUploadHeap
{
    Buffer buffer;
    void* mappedMemory = nullptr;
    uint64_t size = 0;
};

using BaseUploadPool = TGpuResourcePool<VulkanUploadDesc, VulkanGpuMemoryBlock, VulkanUploadHeap, VulkanGpuUploadPoolImpl, VulkanFence>;
class VulkanGpuUploadPoolImpl : public BaseUploadPool
{
public:
    VulkanGpuUploadPoolImpl(VulkanDevice& device, VkQueue queue, uint64_t initialPoolSize)
    : BaseUploadPool(*(new VulkanFence(device, queue)), *this), m_device(device), m_nextHeapSize(initialPoolSize)
    {
    }

    ~VulkanGpuUploadPoolImpl();
    VulkanUploadHeap createNewHeap(const VulkanUploadDesc& desc, uint64_t& outHeapSize);
    void getRange(const VulkanUploadDesc& desc, uint64_t inputOffset, uint64_t& outputOffset, uint64_t& outputSize) const;
    VulkanGpuMemoryBlock allocateHandle(const VulkanUploadDesc& desc, uint64_t heapOffset, const VulkanUploadHeap& heap);
    void destroyHeap(VulkanUploadHeap& heap);
private:
    VulkanDevice& m_device;
    uint64_t m_nextHeapSize = 0;
};

VulkanUploadHeap VulkanGpuUploadPoolImpl::createNewHeap(const VulkanUploadDesc& desc, uint64_t& outHeapSize)
{
    BufferDesc bufferDesc;
    bufferDesc.type = BufferType::Standard;
    bufferDesc.format = Format::RGBA_8_UINT;
    bufferDesc.isConstantBuffer = true;
    bufferDesc.elementCount = std::max(2*desc.requestBytes, m_nextHeapSize);
    bufferDesc.memFlags = (MemFlags)0;
    
    m_nextHeapSize = std::max(2*desc.requestBytes, 2 * m_nextHeapSize);
    outHeapSize = bufferDesc.elementCount;
    auto result = m_device.resources().createBuffer(bufferDesc, ResourceSpecialFlag_CpuUpload);
    CPY_ASSERT(result.success());

    VulkanUploadHeap heap;
    heap.buffer = result;
    heap.size=  bufferDesc.elementCount;

    VkDeviceMemory heapMemory = m_device.resources().unsafeGetResource(heap.buffer).memory;
    VK_OK(vkMapMemory(m_device.vkDevice(), heapMemory, 0u, bufferDesc.elementCount, 0u, &heap.mappedMemory));
    return heap;
}

void VulkanGpuUploadPoolImpl::getRange(const VulkanUploadDesc& desc, uint64_t inputOffset, uint64_t& outOffset, uint64_t& outSize) const
{
    outSize = alignByte(desc.requestBytes, desc.alignment);
    outOffset = alignByte(inputOffset, desc.alignment);
}

VulkanGpuMemoryBlock VulkanGpuUploadPoolImpl::allocateHandle(const VulkanUploadDesc& desc, uint64_t heapOffset, const VulkanUploadHeap& heap)
{
    CPY_ASSERT((heapOffset + desc.requestBytes) <= heap.size);
    CPY_ASSERT((heapOffset % desc.alignment) == 0u);

    VulkanGpuMemoryBlock block = {};
    block.uploadSize = (size_t)alignByte(desc.requestBytes, desc.alignment);
    block.buffer = heap.buffer;
    block.mappedBuffer = static_cast<void*>(static_cast<char*>(heap.mappedMemory) + heapOffset);
    block.offset = heapOffset;
    return block;
}

void VulkanGpuUploadPoolImpl::destroyHeap(VulkanUploadHeap& heap)
{
    m_device.release(heap.buffer);
    heap = {};
}

VulkanGpuUploadPool::VulkanGpuUploadPool(VulkanDevice& device, VkQueue queue, uint64_t initialPoolSize)
{
    m_impl = new VulkanGpuUploadPoolImpl(device, queue, initialPoolSize);
}

VulkanGpuUploadPool::~VulkanGpuUploadPool()
{
    delete m_impl;
}

void VulkanGpuUploadPool::beginUsage()
{
    m_impl->beginUsage();
}

void VulkanGpuUploadPool::endUsage()
{
    m_impl->endUsage();
}

VulkanGpuMemoryBlock VulkanGpuUploadPool::allocUploadBlock(size_t sizeBytes)
{
    VulkanUploadDesc desc;
    desc.alignment = 2560ull;
    desc.requestBytes = sizeBytes;
    return m_impl->allocate(desc);
}

}
}
