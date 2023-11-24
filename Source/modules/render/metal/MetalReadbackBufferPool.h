#pragma once

#include <vector>
#include <coalpy.render/Resources.h>

@protocol MTLBuffer;

namespace coalpy
{
namespace render
{

class MetalDevice;
class MetalResource;

struct MetalReadbackMemBlock
{
    size_t size = 0ull;
    size_t offset = 0ull;
    Buffer buffer = {};
    short allocationId = 0;
    short heapIndex = 0;

    bool valid() const { return buffer.valid(); }
};

class MetalReadbackBufferPool
{
public:
    MetalReadbackBufferPool(MetalDevice& device);
    ~MetalReadbackBufferPool();

    MetalReadbackMemBlock allocate(size_t size);
    void free(const MetalReadbackMemBlock& block);

private:
    MetalDevice& m_device;

    bool createNewHeap(size_t size);

    struct HeapState
    {
        Buffer buffer;
        std::vector<MetalReadbackMemBlock> freeBlocks;
        size_t largestSize = 0ull;
    };

    std::vector<HeapState> m_heaps;
    size_t m_nextHeapSize = 0;
    short m_nextAllocId = 0;
    bool m_isReadback;
};
}
}