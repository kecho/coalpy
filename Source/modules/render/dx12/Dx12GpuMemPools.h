#pragma once

#include <d3d12.h>
#include <coalpy.core/Assert.h>
#include "Dx12Device.h"

namespace coalpy
{
namespace render
{

struct Dx12GpuMemoryBlock
{
    size_t uploadSize = 0ull;
    void* mappedBuffer = nullptr;
    uint64_t offset = 0ull;
    ID3D12Resource* buffer = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS gpuVA = 0;
};

class Dx12GpuUploadPool
{
public:
    Dx12GpuUploadPool(Dx12Device& device, ID3D12CommandQueue& queue, uint64_t initialPoolSize);
    ~Dx12GpuUploadPool();
    void beginUsage();
    void endUsage();

    Dx12GpuMemoryBlock allocUploadBlock(size_t sizeBytes);

private:
    class Dx12GpuUploadPoolImpl* m_impl;
};

struct Dx12GpuDescriptorTable
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    ID3D12DescriptorHeap* ownerHeap = nullptr;
    uint32_t descriptorCounts = 0u;
    uint32_t descriptorSize = 0u;

    bool valid() const
    {
        return cpuHandle.ptr != 0;
    }

    bool isShaderVisible() const
    {
        return gpuHandle.ptr != 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index = 0) const
    {
        CPY_ASSERT(index < descriptorCounts);
        D3D12_CPU_DESCRIPTOR_HANDLE result = { cpuHandle.ptr + index * descriptorSize };
        return result;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(uint32_t index = 0) const
    {
        CPY_ASSERT(index < descriptorCounts);
        D3D12_GPU_DESCRIPTOR_HANDLE result = { gpuHandle.ptr + index * descriptorSize };
        return result;
    }

    void advance(uint32_t count)
    {
        if (count >= descriptorCounts)
            return;
        cpuHandle = getCpuHandle(count);
        gpuHandle = getGpuHandle(count);
        descriptorCounts -= count;
    }
};

class Dx12GpuDescriptorTablePool
{
public:
    Dx12GpuDescriptorTablePool(
        Dx12Device& device,
        ID3D12CommandQueue& queue,
        uint32_t maxDescriptorCount,
        D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    ~Dx12GpuDescriptorTablePool();

    void beginUsage();
    void endUsage();
    Dx12GpuDescriptorTable allocateTable(uint32_t tableSize);
    const Dx12GpuDescriptorTable& lastAllocatedTable() const { return m_lastTable; }

private:
    class Dx12GpuDescriptorTablePoolImpl* m_impl;
    Dx12GpuDescriptorTable m_lastTable;
};

}
}
