#include "VulkanGpuMemPools.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanFence.h"
#include "VulkanFencePool.h"
#include "VulkanUtils.h"
#include "TGpuResourcePool.h"
#include <coalpy.core/Assert.h>
#include <algorithm>

namespace coalpy
{
namespace render
{

class VulkanFenceTimeline
{
public:
    using FenceType = VulkanFenceHandle;

    VulkanFenceTimeline(VulkanDevice& device)
    : m_device(device)
    {
    }

    void beginUsageWithFence(VulkanFenceHandle handle)
    {
        m_currentFenceHandle = handle;
    }

    void waitOnCpu(VulkanFenceHandle handle)
    {
        m_device.fencePool().waitOnCpu(handle);
    }

    void sync()
    {
        //this is done externally
    }

    void signalFence()
    {
        //this is done externally
    } 

    bool isSignaled(VulkanFenceHandle handle)
    {
        return m_device.fencePool().isSignaled(handle);
    }

    VulkanFenceHandle allocateFenceValue()
    {
        return m_currentFenceHandle;
    }

private:
    VulkanFenceHandle m_currentFenceHandle;
    VulkanDevice& m_device;
    std::vector<VulkanFenceHandle> m_activeFences;
};

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

using BaseUploadPool = TGpuResourcePool<VulkanUploadDesc, VulkanGpuMemoryBlock, VulkanUploadHeap, VulkanGpuUploadPoolImpl, VulkanFenceTimeline>;

class VulkanGpuUploadPoolImpl : public BaseUploadPool, public VulkanFenceTimeline
{
public:
    VulkanGpuUploadPoolImpl(VulkanDevice& device, VkQueue queue, uint64_t initialPoolSize)
    : VulkanFenceTimeline(device), BaseUploadPool(*this, *this), m_device(device), m_nextHeapSize(initialPoolSize)
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

void VulkanGpuUploadPool::beginUsage(VulkanFenceHandle handle)
{
    m_impl->beginUsageWithFence(handle);
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


VulkanGpuDescriptorSetPool::VulkanGpuDescriptorSetPool(VulkanDevice& device, VkQueue queue)
: m_device(device), m_fence(*(new VulkanFence(device, queue))), m_nextFenceVal(0ull), m_activePool(-1)
{
}

VulkanGpuDescriptorSetPool::~VulkanGpuDescriptorSetPool()
{
    delete &m_fence;
}

VkDescriptorPool VulkanGpuDescriptorSetPool::newPool() const
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 64 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64 }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr };
    poolInfo.flags = {};
    poolInfo.maxSets = 32;
    poolInfo.poolSizeCount = (int)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    
    VkDescriptorPool newPool = {};
    VK_OK(vkCreateDescriptorPool(m_device.vkDevice(), &poolInfo, nullptr, &newPool));
    return newPool;
}

void VulkanGpuDescriptorSetPool::beginUsage()
{
    uint64_t completeValue = m_fence.completedValue();
    while (!m_livePools.empty())
    {
        auto& candidate = m_pools[m_livePools.front()];
        if (candidate.fenceVal > completeValue)
            break;

        m_freePools.push(m_livePools.front());
        m_livePools.pop();
    }

    if (!m_freePools.empty())
    {
        m_activePool = m_freePools.front();
        m_freePools.pop();
        VK_OK(vkResetDescriptorPool(m_device.vkDevice(), m_pools[m_activePool].pool, 0));
    }
    else
    {
        m_activePool = m_pools.size();
        m_pools.push_back(PoolState { newPool(), 0ull });
    }
}

void VulkanGpuDescriptorSetPool::endUsage()
{
    ++m_nextFenceVal;
    m_fence.signal(m_nextFenceVal);
    m_pools[m_activePool].fenceVal = m_nextFenceVal;
    m_livePools.push(m_activePool);
    m_activePool = -1;
}

VkDescriptorSet VulkanGpuDescriptorSetPool::allocUploadBlock(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocationInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
    allocationInfo.descriptorSetCount = 1;
    allocationInfo.pSetLayouts = &layout;
    
    VkDescriptorSet outSet = {};
    bool foundAllocation = false;
    while (!foundAllocation)
    {
        allocationInfo.descriptorPool = m_pools[m_activePool].pool;
        if (vkAllocateDescriptorSets(m_device.vkDevice(), &allocationInfo, &outSet) == VK_SUCCESS)
        {
            foundAllocation = true;
            continue;
        }

        endUsage();
        beginUsage();
    }

    return outSet;
}

}
}
