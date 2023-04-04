#pragma once

#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/HandleContainer.h>
#include <d3d12.h>
#include <shared_mutex>

namespace coalpy
{
namespace render
{

class Dx12Device;

struct Dx12CounterHandle : public GenericHandle<int> {};

class Dx12CounterPool
{
public:
    enum 
    {
        MaxCounters = 64
    };

    Dx12CounterPool(Dx12Device& device);
    ~Dx12CounterPool() {}

    ID3D12Resource& resource() { return *m_resource; }

    Dx12CounterHandle allocate();
    void free(Dx12CounterHandle);
    int counterOffset(Dx12CounterHandle handle) const;

private:
    mutable std::shared_mutex m_mutex;

    struct CounterSlot
    {
        int offset = 0;
    };

    Dx12Device& m_device;
    HandleContainer<Dx12CounterHandle, CounterSlot, (int)MaxCounters> m_counters;
    SmartPtr<ID3D12Resource> m_resource;

    D3D12_UNORDERED_ACCESS_VIEW_DESC m_uavDesc;
};

}
}
