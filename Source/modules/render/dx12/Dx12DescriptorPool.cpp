#include <Config.h>

#if ENABLE_DX12

#include <algorithm>
#include <coalpy.core/Assert.h>
#include "Dx12DescriptorPool.h"
#include "Dx12Device.h"

namespace coalpy
{
namespace render
{

Dx12DescriptorPool::Dx12DescriptorPool(Dx12Device& device)
: m_device(device)
{
    for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        Dx12DescriptorHeap& container = m_heaps[i];
        auto& desc = container.desc;
        desc.Type = (D3D12_DESCRIPTOR_HEAP_TYPE)i;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        UINT tableTypeCounts = 0;

        switch(desc.Type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            desc.NumDescriptors = 1024;
            tableTypeCounts = 20000;
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            desc.NumDescriptors = 1024;
            tableTypeCounts = 20000;
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            desc.NumDescriptors = 1024;
            tableTypeCounts = 20000;
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            desc.NumDescriptors = 1024;
            tableTypeCounts = 0;
            break;
        default:
            break;
        }

        container.incrSize = m_device.device().GetDescriptorHandleIncrementSize(desc.Type);
        DX_OK(m_device.device().CreateDescriptorHeap(
                &desc, DX_RET(container.heap)));
        
        Dx12DescriptorTableType tableType = getTableType(desc.Type);
        if (tableType != Dx12DescriptorTableType::Invalid && tableTypeCounts > 0)
        {
            auto& tableHeap = m_tables[(int)tableType];
            tableHeap.desc = {};
            tableHeap.desc.Type = desc.Type;
            tableHeap.desc.NumDescriptors = tableTypeCounts;
            tableHeap.desc.NodeMask = 0;
            tableHeap.lastIndex = 0;
            tableHeap.incrSize = m_device.device().GetDescriptorHandleIncrementSize(
                tableHeap.desc.Type);
            DX_OK(m_device.device().CreateDescriptorHeap(
                &tableHeap.desc,
                DX_RET(tableHeap.heap)));
        }
    }
}

Dx12Descriptor Dx12DescriptorPool::allocInternal(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    CPY_ASSERT(type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
    auto& container = m_heaps[type];
    auto& freeList = container.freeSpots;
    auto& desc = container.desc;
    UINT cpuRequestedIndex = 0;
    if (freeList.empty())
    {
        if (container.lastIndex >= desc.NumDescriptors)
            CPY_ERROR_MSG(false, "Not enough dx12 descriptors!");
        cpuRequestedIndex = std::min<UINT>(container.lastIndex++, desc.NumDescriptors);
    }
    else
    {
        cpuRequestedIndex = freeList.back();
        freeList.pop_back();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = container.heap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += container.incrSize * cpuRequestedIndex;
    return Dx12Descriptor { desc.Type, cpuHandle, cpuRequestedIndex };
}

void Dx12DescriptorPool::release(const Dx12Descriptor& descriptor)
{
    CPY_ASSERT(descriptor.type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
    auto& container = m_heaps[descriptor.type];
    container.freeSpots.push_back(descriptor.index);
}

Dx12DescriptorTable Dx12DescriptorPool::allocateTable(
    Dx12DescriptorTableType tableType,
    const Dx12Descriptor* cpuHandles, UINT cpuHandleCounts)
{
    Dx12DescriptorTable resultTable = allocEmptyTable(cpuHandleCounts, tableType);
    auto& tableHeap = m_tables[(int)tableType];
    for (UINT i = 0; i < cpuHandleCounts; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE tableDescriptor = resultTable.baseHandle;
        tableDescriptor.ptr += i * tableHeap.incrSize;
        m_device.device().CopyDescriptorsSimple(
            1, tableDescriptor, cpuHandles[i].handle, getHeapType(tableType));
    }
    return resultTable;
}

Dx12DescriptorTable Dx12DescriptorPool::allocateTableCopy(const Dx12DescriptorTable& srcTable)
{
    Dx12DescriptorTable resultTable = allocEmptyTable(srcTable.count, srcTable.tableType);
    m_device.device().CopyDescriptorsSimple(
        srcTable.count,
        resultTable.baseHandle,
        srcTable.baseHandle,
        getHeapType(srcTable.tableType));
    return resultTable;
}

Dx12DescriptorTable Dx12DescriptorPool::allocEmptyTable(UINT count, Dx12DescriptorTableType tableType)
{
    CPY_ASSERT(count > 0);
    Dx12DescriptorTable resultTable;

    auto& tableHeap = m_tables[(int)tableType];
    //for now linearly find a heap that fits
    for (int i = 0; i < (int)tableHeap.freeSpots.size(); ++i)
    {
        auto& freeTable = tableHeap.freeSpots[i];
        if (count == freeTable.count)
        {
            resultTable = freeTable;
            resultTable.tableType = tableType;
            tableHeap.freeSpots[i] = tableHeap.freeSpots[(int)tableHeap.freeSpots.size() - 1];
            tableHeap.freeSpots.pop_back();
            return resultTable;
        }
    }

    //if not check if theres enough size
    UINT remaining = tableHeap.desc.NumDescriptors - tableHeap.lastIndex;
    CPY_ERROR_MSG(count <= remaining, "Not enough memory remaining for descriptors!");
    resultTable.tableType = tableType;
    resultTable.count = count;
    resultTable.baseHandle = tableHeap.heap->GetCPUDescriptorHandleForHeapStart();
    resultTable.baseHandle.ptr += tableHeap.incrSize * tableHeap.lastIndex;
    resultTable.guid = m_nextTableGuid++;
    tableHeap.lastIndex += count;
    return resultTable;
}

void Dx12DescriptorPool::release(const Dx12DescriptorTable& table)
{
    CPY_ASSERT((int)table.tableType >= 0 && (int)table.tableType < (int)Dx12DescriptorTableType::Count);
    m_tables[(int)table.tableType].freeSpots.push_back(table);
}

Dx12DescriptorPool::~Dx12DescriptorPool()
{
}

}
}

#endif
