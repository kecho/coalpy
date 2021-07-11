#include <Config.h>
#if ENABLE_DX12

#include "Dx12Queues.h"
#include "Dx12Device.h"
#include "Dx12Fence.h"
#include "Dx12GpuMemPools.h"
#include <D3D12.h>
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

namespace
{

static D3D12_COMMAND_LIST_TYPE translateQueueType(WorkType workType)
{
    static const D3D12_COMMAND_LIST_TYPE s_cmdListTypes[(int)WorkType::Count] = {
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        D3D12_COMMAND_LIST_TYPE_COPY
    };

    return s_cmdListTypes[(unsigned)workType];
}

}

Dx12Queues::Dx12Queues(Dx12Device& device)
: m_device(device)
{
    for (int queueIt = 0u; queueIt < (int)WorkType::Count; ++queueIt)
    {
        QueueContainer& qcontainer = m_containers[queueIt];

        D3D12_COMMAND_QUEUE_DESC qDesc = {
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0 /*node mask*/
        };

        qDesc.Type = translateQueueType((WorkType)queueIt);
        DX_OK(m_device.device().CreateCommandQueue(&qDesc, DX_RET(qcontainer.queue)));

        if (qcontainer.queue)
            qcontainer.fence = new Dx12Fence(device, *qcontainer.queue);

        qcontainer.memPools.uploadPool = new Dx12GpuUploadPool(device, *qcontainer.queue, 4 * 1024 * 1024);
        qcontainer.memPools.tablePool = new Dx12GpuDescriptorTablePool(device, *qcontainer.queue, 512, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

Dx12Queues::~Dx12Queues()
{
    for (auto& q : m_containers)
    {
        //wait for any pending command list.
        for (auto& allocatorRec : q.allocatorPool)
            q.fence->waitOnCpu(allocatorRec.fenceValue);

        delete q.fence;
        if (q.queue)
            q.queue->Release();

        delete q.memPools.uploadPool;
        delete q.memPools.tablePool;
    }
}

void Dx12Queues::allocate(WorkType type, Dx12List& outList)
{
    CPY_ASSERT((int)type >= 0 && (int)type < (int)WorkType::Count);
    auto& container = m_containers[(int)type];

    UINT64 currentFenceValue = container.fence->value();
    SmartPtr<ID3D12CommandAllocator> newAllocator;
    //find an allocator candidate that is free
    for (int i = 0; i < (int)container.allocatorPool.size(); ++i)
    {
        auto& record = container.allocatorPool[i];
        if (record.fenceValue <= currentFenceValue)
        {
            newAllocator = record.allocator;
            record = container.allocatorPool[container.allocatorPool.size() - 1];
            container.allocatorPool.pop_back();
            break;
        }
    }

    auto dx12ListType = getDx12WorkType(type);
    if (newAllocator == nullptr)
        DX_OK(m_device.device().CreateCommandAllocator(dx12ListType, DX_RET(newAllocator)));

    SmartPtr<ID3D12GraphicsCommandList6> newList;
    if (!container.listPool.empty())
    {
        newList = container.listPool.back();
        container.listPool.pop_back();
        DX_OK(newList->Reset(&(*newAllocator), nullptr));
    }
    else
    {
        DX_OK(m_device.device().CreateCommandList(0u, dx12ListType, &(*newAllocator), nullptr, DX_RET(newList)));
    }

    outList.workType = type;
    outList.list = newList;
    outList.allocator = newAllocator;
}

void Dx12Queues::deallocate(Dx12List& list, UINT64 fenceValue)
{
    CPY_ASSERT((int)list.workType >= 0 && (int)list.workType < (int)WorkType::Count);
    auto& container = m_containers[(int)list.workType];

    container.listPool.push_back(list.list);
    AllocatorRecord newRecord = { fenceValue, list.allocator };
    container.allocatorPool.push_back(newRecord);
    
    list = {};
}

UINT64 Dx12Queues::signalFence(WorkType type)
{
    CPY_ASSERT((int)type >= 0 && (int)type < (int)WorkType::Count);
    auto& container = m_containers[(int)type];
    return container.fence->signal();
}

UINT64 Dx12Queues::currentFenceValue(WorkType type)
{
    CPY_ASSERT((int)type >= 0 && (int)type < (int)WorkType::Count);
    auto& container = m_containers[(int)type];
    return container.fence->value();
}

}
}

#endif
