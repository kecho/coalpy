#pragma once

#include <coalpy.core/SmartPtr.h>
#include <vector>

struct ID3D12Resource;

namespace coalpy
{

namespace render
{

class Dx12Buffer;
class Dx12Device;

struct Dx12CpuMemBlock
{
    size_t size = 0ull;
    size_t offset = 0ull;
    void* mappedMemory = nullptr;
    ID3D12Resource* buffer = nullptr;
    short allocationId = 0;
    short heapIndex = 0;

    bool valid() const { return buffer != nullptr; }
};

class Dx12BufferPool
{
public:
    Dx12BufferPool(Dx12Device& device, bool isReadback);
    ~Dx12BufferPool();

    Dx12CpuMemBlock allocate(size_t size);
    void free(const Dx12CpuMemBlock& block);

private:
    Dx12Device& m_device;

    struct HeapState
    {
        Dx12Buffer* buffer;
        std::vector<Dx12CpuMemBlock> freeBlocks;
        size_t largestSize = 0ull;
    };

    bool createNewHeap(size_t size);

    std::vector<HeapState> m_heaps;
    size_t m_nextHeapSize = 0;
    short m_nextAllocId = 0;
    bool m_isReadback;
};

}

}
