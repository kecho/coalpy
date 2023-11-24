#include <Config.h>
#if ENABLE_METAL

#include <Metal/Metal.h>
#include "MetalReadbackBufferPool.h"
#include "MetalDevice.h"
#include "MetalResources.h"

namespace coalpy
{
namespace render
{

enum 
{
    InitialBufferPoolSize = 1024 * 1024 * 5 //5 megabytes of initial size
};

bool MetalReadbackBufferPool::createNewHeap(size_t size)
{
    short heapIndex = (short)m_heaps.size();
    m_heaps.emplace_back();
    HeapState& heap = m_heaps.back();

    BufferDesc bufferDesc;
    bufferDesc.type = BufferType::Standard;
    bufferDesc.format = Format::RGBA_8_UINT;
    bufferDesc.elementCount = size;
    bufferDesc.memFlags = (MemFlags)0;
    ResourceSpecialFlags readbackFlags = ResourceSpecialFlag_CpuReadback;
    BufferResult result = m_device.resources().createBuffer(bufferDesc, readbackFlags);
    if (!result.success())
        return false;

    heap.buffer = result;
    heap.largestSize = size;
    MetalReadbackMemBlock memBlock;
    memBlock.size = size;
    memBlock.offset = 0;
    memBlock.buffer = result;
    memBlock.allocationId = m_nextAllocId++;
    memBlock.heapIndex = heapIndex;

    return true;
}

MetalReadbackBufferPool::MetalReadbackBufferPool(MetalDevice& device)
: m_device(device)
{
    bool success = createNewHeap(InitialBufferPoolSize);
    CPY_ASSERT_MSG(success, "Could not allocate heap.");
    m_nextHeapSize = 2 * InitialBufferPoolSize;
}

MetalReadbackBufferPool::~MetalReadbackBufferPool()
{
    for (auto& h : m_heaps)
    {
        m_device.resources().release(h.buffer);
        CPY_ASSERT_MSG(h.freeBlocks.size() == 1, "Warning: non freed heap elements found. Buffer pool has some memory blocks that have not been freed.");
    }
    m_heaps.clear();
}

}
}
#endif // ENABLE_METAL