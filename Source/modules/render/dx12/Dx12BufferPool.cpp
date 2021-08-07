#include "Dx12BufferPool.h"
#include <coalpy.core/Formats.h>
#include <coalpy.core/Assert.h>
#include "Dx12Resources.h"
#include <algorithm>

namespace coalpy
{
namespace render
{

enum 
{
    InitialBufferPoolSize = 1024 * 1024 * 5 //5 megabytes of initial size
};

Dx12BufferPool::Dx12BufferPool(Dx12Device& device, bool isReadback)
: m_device(device), m_isReadback(isReadback)
{
    bool success = createNewHeap(InitialBufferPoolSize);
    CPY_ASSERT_MSG(success, "Could not allocate heap.");
    m_nextHeapSize = 2 * InitialBufferPoolSize;
}

bool Dx12BufferPool::createNewHeap(size_t size)
{
    short heapIndex = (short)m_heaps.size();
    m_heaps.emplace_back();
    HeapState& heap = m_heaps.back();

    BufferDesc bufferDesc;
    bufferDesc.type = BufferType::Standard;
    bufferDesc.format = Format::RGBA_8_UINT;
    bufferDesc.elementCount = size;
    bufferDesc.memFlags = (MemFlags)0;
    ResourceSpecialFlags readbackFlags = m_isReadback ? ResourceSpecialFlag_CpuReadback : ResourceSpecialFlag_CpuUpload;
    heap.buffer = new Dx12Buffer(m_device, bufferDesc, (ResourceSpecialFlags)(ResourceSpecialFlag_NoDeferDelete | ResourceSpecialFlag_CanDenyShaderResources | readbackFlags));
    Dx12ResourceInitResult initResult = heap.buffer->init();
    if (!initResult.success())
    {
        CPY_ASSERT_FMT(false, "Failed creating heap: %s", initResult.message.c_str());
        return false;
    }
    
    heap.largestSize = size;

    Dx12CpuMemBlock memBlock;
    memBlock.size = size;
    memBlock.mappedMemory = heap.buffer->mappedMemory();
    memBlock.offset = 0;
    memBlock.buffer = &heap.buffer->d3dResource();
    memBlock.allocationId = m_nextAllocId++;
    memBlock.heapIndex = heapIndex;
    heap.freeBlocks.push_back(memBlock);
    return true;
}

Dx12BufferPool::~Dx12BufferPool()
{
    for (auto& h : m_heaps)
    {
        h.buffer->Release();
        CPY_ASSERT_MSG(h.freeBlocks.size() == 1, "Warning: non freed heap elements found. Buffer pool has some memory blocks that have not been freed.");
    }
    m_heaps.clear();
}

Dx12CpuMemBlock Dx12BufferPool::allocate(size_t size)
{
    int selectedHeapIndex = -1;
    //find smallest most fit heap
    for (int heapIndex = 0; heapIndex < (int)m_heaps.size(); ++heapIndex)
    {
        HeapState& heap = m_heaps[heapIndex];
        if (heap.largestSize >= size && (selectedHeapIndex == -1 || (m_heaps[selectedHeapIndex].largestSize > heap.largestSize)))
            selectedHeapIndex = heapIndex;
    }

    if (selectedHeapIndex == -1)
    {
        selectedHeapIndex = m_heaps.size();
        if (!createNewHeap(m_nextHeapSize))
            return Dx12CpuMemBlock();

        m_nextHeapSize = (2 * m_nextHeapSize) > size ? (2 * m_nextHeapSize) : size;
    }

    HeapState& selectedHeap = m_heaps[selectedHeapIndex];

    int bestFreeSpot = -1;
    for (int freeSpotIndex = 0; freeSpotIndex < (int)selectedHeap.freeBlocks.size(); ++freeSpotIndex)
    {
        Dx12CpuMemBlock& freeSpot = selectedHeap.freeBlocks[freeSpotIndex];
        if (freeSpot.size >= size && (bestFreeSpot == -1 || (selectedHeap.freeBlocks[bestFreeSpot].size > freeSpot.size)))
            bestFreeSpot = freeSpotIndex;
    }

    CPY_ASSERT_MSG(bestFreeSpot != -1, "Inconsistency in heaps. This should not happen: at this point we should find a free segment of the heap.");
    if (bestFreeSpot == -1)
        return Dx12CpuMemBlock();
    
    Dx12CpuMemBlock memBlock = selectedHeap.freeBlocks[bestFreeSpot];
    size_t foundBlockSize = memBlock.size;
    Dx12CpuMemBlock partition = memBlock;
    partition.size -= size;
    partition.offset += size;
    partition.mappedMemory = (char*)partition.mappedMemory + size;
    memBlock.size = size;

    if (partition.size == 0)
    {
        selectedHeap.freeBlocks[bestFreeSpot] = selectedHeap.freeBlocks.back();
        selectedHeap.freeBlocks.pop_back();
    }
    else
    {
        selectedHeap.freeBlocks[bestFreeSpot] = partition;
    }

    if (foundBlockSize == selectedHeap.largestSize)
    {
        for (auto& f : selectedHeap.freeBlocks)
            selectedHeap.largestSize = selectedHeap.largestSize > f.size ? selectedHeap.largestSize : f.size;
    }

    return memBlock;
}

void Dx12BufferPool::free(const Dx12CpuMemBlock& block)
{
    int heapIndex = (int)block.heapIndex;
    CPY_ASSERT_FMT(heapIndex >= 0  && heapIndex < (int)m_heaps.size(), "Heap index used %d is corrupted.", heapIndex);
    if (heapIndex < 0 || heapIndex >= (int)m_heaps.size())
        return;

    HeapState& state = m_heaps[heapIndex];
    auto& freeBlocks = state.freeBlocks;
    int beforeIndexToMerge = -1;
    int afterIndexToMerge = -1;
    for (int i = 0; i < freeBlocks.size(); ++i)
    {
        auto& candidateBlock = freeBlocks[i];
        if (beforeIndexToMerge == -1 && (candidateBlock.offset + candidateBlock.size) == block.offset)
            beforeIndexToMerge = i;
        if (afterIndexToMerge == -1 && (block.offset + block.size) == candidateBlock.offset)
            afterIndexToMerge = i;
    
        if (beforeIndexToMerge != -1 && afterIndexToMerge != -1)
            break;
    }

    size_t newHeapSize = block.size;
    if (beforeIndexToMerge != -1 && afterIndexToMerge != -1)
    {
        auto& beforeBlockToMerge = freeBlocks[beforeIndexToMerge];
        auto& afterBlockToMerge = freeBlocks[afterIndexToMerge];
        beforeBlockToMerge.size += block.size + freeBlocks[afterIndexToMerge].size;
        afterBlockToMerge = freeBlocks.back();
        newHeapSize = newHeapSize > beforeBlockToMerge.size ? newHeapSize : beforeBlockToMerge.size;
        freeBlocks.pop_back();
    }
    else if (beforeIndexToMerge != -1)
    {
        auto& beforeBlockToMerge = freeBlocks[beforeIndexToMerge];
        beforeBlockToMerge.size += block.size;
        newHeapSize = newHeapSize > beforeBlockToMerge.size ? newHeapSize : beforeBlockToMerge.size;
    }
    else if (afterIndexToMerge != -1)
    {
        auto& afterBlockToMerge = freeBlocks[afterIndexToMerge];
        afterBlockToMerge.size += block.size;
        afterBlockToMerge.offset -= block.size;
        afterBlockToMerge.mappedMemory = (char*)afterBlockToMerge.mappedMemory - block.size;
        newHeapSize = newHeapSize > afterBlockToMerge.size ? newHeapSize : afterBlockToMerge.size;
    }
    else
    {
        freeBlocks.push_back(block);
    }
    state.largestSize = state.largestSize > newHeapSize ? state.largestSize : newHeapSize;
}

}
}
