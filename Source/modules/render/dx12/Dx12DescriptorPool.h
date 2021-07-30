#pragma once

#include <coalpy.core/SmartPtr.h>
#include <d3d12.h>
#include <vector>

namespace coalpy
{
namespace render
{

class Dx12Device;

enum class Dx12DescriptorTableType
{
    SrvCbvUav,
    Sampler,
    Rtv,
    Count,
    Invalid
};

inline Dx12DescriptorTableType getTableType(D3D12_DESCRIPTOR_HEAP_TYPE heapType) 
{
    switch(heapType)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return Dx12DescriptorTableType::SrvCbvUav;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return Dx12DescriptorTableType::Sampler;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        return Dx12DescriptorTableType::Rtv;
    }
    return Dx12DescriptorTableType::Invalid;
}

inline D3D12_DESCRIPTOR_HEAP_TYPE getHeapType(Dx12DescriptorTableType tt)
{
    switch(tt)
    {
    case Dx12DescriptorTableType::SrvCbvUav:
        return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case Dx12DescriptorTableType::Sampler:
        return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    case Dx12DescriptorTableType::Rtv:
        return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    }
    return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
}

struct Dx12Descriptor
{
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    UINT index;
};

struct Dx12DescriptorTable
{
    D3D12_CPU_DESCRIPTOR_HANDLE baseHandle = {};
    Dx12DescriptorTableType tableType = Dx12DescriptorTableType::SrvCbvUav;
    UINT guid = 0;
    UINT count = 0;
};

class Dx12DescriptorPool
{
public:
    Dx12DescriptorPool(Dx12Device& device);
    ~Dx12DescriptorPool();

    inline Dx12Descriptor allocateSrvOrUavOrCbv()
    {
        return allocInternal(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    inline Dx12Descriptor allocateRtv()
    {
        return allocInternal(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    Dx12DescriptorTable allocateTable(
        Dx12DescriptorTableType type,
        const Dx12Descriptor* cpuHandles, UINT cpuHandleCounts);

    Dx12DescriptorTable allocateTableCopy(const Dx12DescriptorTable& srcTable);

    void release(const Dx12Descriptor& descriptor);
    void release(const Dx12DescriptorTable& table);

private:
    Dx12Descriptor allocInternal(D3D12_DESCRIPTOR_HEAP_TYPE type);
    Dx12DescriptorTable allocEmptyTable(UINT count, Dx12DescriptorTableType tableType);

    struct Dx12DescriptorHeap
    {
        UINT incrSize = 0;
        UINT lastIndex = 0;
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        SmartPtr<ID3D12DescriptorHeap> heap;
        std::vector<UINT> freeSpots;
    };

    struct Dx12DescriptorTableHeap 
    {
        UINT incrSize = 0;
        UINT lastIndex = 0;
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        SmartPtr<ID3D12DescriptorHeap> heap;
        std::vector<Dx12DescriptorTable> freeSpots;
    };

    UINT m_nextTableGuid = 0;
    Dx12DescriptorHeap m_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    Dx12DescriptorTableHeap m_tables[(int)Dx12DescriptorTableType::Count];
    Dx12Device& m_device;
};

}
}
