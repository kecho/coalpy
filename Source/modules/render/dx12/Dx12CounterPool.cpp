#include "Dx12CounterPool.h"
#include "Dx12Utils.h"
#include "Dx12Device.h"
#include "Config.h"
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

Dx12CounterPool::Dx12CounterPool(Dx12Device& device)
: m_device(device)
{
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * MaxCounters; //offsets must be unfortunately 4096 bytes apart -_-. Thats 2kb waste per counter.
    desc.Height  = 1;
    desc.DepthOrArraySize  = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1u;
    desc.SampleDesc.Quality = 0u;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 0u;
    heapProps.VisibleNodeMask = 0u;
	auto heapFlags = D3D12_HEAP_FLAG_NONE;

    HRESULT r = m_device.device().CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON, //This must match the same resource state in dx12Resources.cpp
        nullptr, DX_RET(m_resource));
    CPY_ASSERT_MSG(r == S_OK, "Failed creating counter resource ints pool.");

    m_uavDesc.Format = DXGI_FORMAT_R32_UINT;
    m_uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    m_uavDesc.Buffer.FirstElement = 0;
    m_uavDesc.Buffer.NumElements = 1;
    m_uavDesc.Buffer.StructureByteStride = sizeof(int);
    m_uavDesc.Buffer.CounterOffsetInBytes = 0;
    m_uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC Dx12CounterPool::uavDesc(Dx12CounterHandle handle) const
{
    const bool isValid = handle.valid() && m_counters.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return {};

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav = m_uavDesc;
    uav.Buffer.FirstElement = (handle.handleId * D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT) / 4;
    uav.Buffer.NumElements = 1u;
    return uav;
}

Dx12CounterHandle Dx12CounterPool::allocate()
{
    Dx12CounterHandle handle;
    CounterSlot& slot = m_counters.allocate(handle);
    if (handle.valid())
        slot.offset = handle.handleId * D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;

    return handle;
}

int Dx12CounterPool::counterOffset(Dx12CounterHandle handle) const
{
    const bool isValid = handle.valid() && m_counters.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return 0;

    return m_counters[handle].offset;
}

void Dx12CounterPool::free(Dx12CounterHandle handle)
{
    const bool isValid = handle.valid() && m_counters.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;

    m_counters.free(handle);
}


}
}
