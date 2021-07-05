#pragma once

#include <d3d12.h>
#include <coalpy.core/SmartPtr.h>
#include <vector>

namespace coalpy
{
namespace render
{

class Dx12Device;
class Dx12Fence;

enum class WorkType
{
    Graphics,
    Compute,
    Copy,
    Count
};

struct Dx12List
{
    WorkType workType = WorkType::Graphics;
    SmartPtr<ID3D12GraphicsCommandList6> list;
    SmartPtr<ID3D12CommandAllocator> allocator;
};

inline D3D12_COMMAND_LIST_TYPE getDx12WorkType(WorkType type)
{
    switch (type)
    {
    case WorkType::Graphics:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case WorkType::Compute:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case WorkType::Copy:
        return D3D12_COMMAND_LIST_TYPE_COPY;
    default:
        CPY_ASSERT_FMT(false, "Invalid work type utilized %d", (int)type);
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

class Dx12Queues
{
public:
    Dx12Queues(Dx12Device& device);
    ~Dx12Queues();

    ID3D12CommandQueue& cmdQueue(WorkType type) { return *(m_containers[(int)type].queue); }
    
    void allocate(WorkType workType, Dx12List& outList);
    UINT64 signalFence(WorkType workType);
    void deallocate(Dx12List& list, UINT64 fenceValue);

private:

    struct AllocatorRecord
    {
        UINT64 fenceValue = 0;
        SmartPtr<ID3D12CommandAllocator> allocator;
    };

    struct QueueContainer
    {
        ID3D12CommandQueue* queue = nullptr;
        Dx12Fence* fence = nullptr;
        std::vector<AllocatorRecord> allocatorPool;
        std::vector<SmartPtr<ID3D12GraphicsCommandList6>> listPool;
    };

    QueueContainer m_containers[(int)WorkType::Count];
    Dx12Device& m_device;
};

}
}
