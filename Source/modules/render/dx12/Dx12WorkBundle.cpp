#include "Dx12WorkBundle.h"
#include "Dx12Device.h"
#include "Dx12Resources.h"
#include "Dx12ResourceCollection.h"
#include "Dx12Queues.h"

namespace coalpy
{
namespace render
{

bool Dx12WorkBundle::load(const WorkBundle& workBundle)
{
    m_workBundle = workBundle;
    for (auto& it : workBundle.states)
        m_states[it.first] = getDx12GpuState(it.second.state);

    return true;
}

void Dx12WorkBundle::uploadAllTables()
{
    int totalDescriptors = m_workBundle.totalTableSize + m_workBundle.totalConstantBuffers;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescBase;
    srcDescBase.reserve(totalDescriptors);

    std::vector<UINT> srcDescCounts;
    srcDescCounts.reserve(totalDescriptors);

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dstDescBase;
    dstDescBase.reserve(totalDescriptors);

    std::vector<UINT> dstDescCounts;
    dstDescCounts.reserve(totalDescriptors);

    Dx12ResourceCollection& resources = m_device.resources();
    if (m_srvUavTable.ownerHeap != nullptr)
    {
        for (auto& it : m_workBundle.tableAllocations)
        {
            CPY_ASSERT(it.first.valid());
            Dx12ResourceTable& t = resources.unsafeGetTable(it.first);
            const Dx12DescriptorTable& cpuTable = t.cpuTable();
            
            srcDescBase.push_back(cpuTable.baseHandle);
            srcDescCounts.push_back(cpuTable.count);

            dstDescBase.push_back(m_srvUavTable.getCpuHandle(it.second.offset));
            dstDescCounts.push_back(it.second.count);
            CPY_ASSERT(it.second.count == cpuTable.count);
        }
    }

    if (m_cbvTable.ownerHeap != nullptr)
    {
        for (auto& it : m_workBundle.constantDescriptorOffsets)
        {
            CPY_ASSERT(it.first.valid());
            Dx12Buffer& r = (Dx12Buffer&)resources.unsafeGetResource(it.first);
            CPY_ASSERT(r.bufferDesc().isConstantBuffer);
            if (!r.bufferDesc().isConstantBuffer)
                continue;

            srcDescBase.push_back(r.cbv().handle);
            srcDescCounts.push_back(1);

            dstDescBase.push_back(m_cbvTable.getCpuHandle(it.second));
            dstDescCounts.push_back(1);
        }
    }

    if (!dstDescBase.empty())
    {
        m_device.device().CopyDescriptors(
            (UINT)dstDescBase.size(),
            dstDescBase.data(), dstDescCounts.data(),
            (UINT)srcDescBase.size(),
            srcDescBase.data(), srcDescCounts.data(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

void Dx12WorkBundle::applyBarriers(const std::vector<ResourceBarrier>& barriers, ID3D12GraphicsCommandList6& outList)
{
    if (barriers.empty())
        return;

    std::vector<D3D12_RESOURCE_BARRIER> resultBarriers;
    resultBarriers.reserve(barriers.size());
    Dx12ResourceCollection& resources = m_device.resources();
    for (const auto& b : barriers)
    {
        if (b.prevState == b.postState)
            continue;

        Dx12Resource& r = resources.unsafeGetResource(b.resource);
        
        resultBarriers.emplace_back();
        D3D12_RESOURCE_BARRIER& d3d12barrier = resultBarriers.back();
        d3d12barrier = {};
        d3d12barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        if (b.type == BarrierType::Begin)
        {
            d3d12barrier.Flags |= D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
        }
        else if (b.type == BarrierType::End)
        {
            d3d12barrier.Flags |= D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
        }

        d3d12barrier.Transition.pResource = &r.d3dResource();
        d3d12barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        d3d12barrier.Transition.StateBefore = getDx12GpuState(b.prevState);
        d3d12barrier.Transition.StateAfter  = getDx12GpuState(b.postState);
    }

    if (resultBarriers.empty())
        return;

    outList.ResourceBarrier((UINT)resultBarriers.size(), resultBarriers.data());
}

void Dx12WorkBundle::buildCommandList(int listIndex, const CommandList* cmdList, ID3D12GraphicsCommandList6& outList)
{
    CPY_ASSERT(cmdList->isFinalized());
    const unsigned char* listData = cmdList->data();
    const ProcessedList& pl = m_workBundle.processedLists[listIndex];
    for (int commandIndex = 0; commandIndex < pl.commandSchedule.size(); ++commandIndex)
    {
        const CommandInfo& cmdInfo = pl.commandSchedule[commandIndex];
        const unsigned char* cmdBlob = listData + cmdInfo.commandOffset;
        AbiCmdTypes cmdType = *((AbiCmdTypes*)cmdBlob);
        applyBarriers(cmdInfo.preBarrier, outList);
        switch (cmdType)
        {
        case AbiCmdTypes::Compute:
            {
                const auto* abiCmd = (const AbiComputeCmd*)cmdBlob;
            }
            break;
        case AbiCmdTypes::Copy:
            {
                const auto* abiCmd = (const AbiCopyCmd*)cmdBlob;
            }
            break;
        case AbiCmdTypes::Upload:
            {
                const auto* abiCmd = (const AbiUploadCmd*)cmdBlob;
            }
            break;
        case AbiCmdTypes::Download:
            {
                const auto* abiCmd = (const AbiDownloadCmd*)cmdBlob;
            }
            break;
        default:
            CPY_ASSERT_FMT(false, "Unrecognized serialized command %d", cmdType);
            return;
        }
        applyBarriers(cmdInfo.postBarrier, outList);
    }
}

void Dx12WorkBundle::execute(CommandList** commandLists, int commandListsCount)
{
    CPY_ASSERT(commandListsCount == (int)m_workBundle.processedLists.size());

    WorkType workType = WorkType::Graphics;
    auto& queues = m_device.queues();
    ID3D12CommandQueue& cmdQueue = queues.cmdQueue(workType);
    Dx12MemoryPools& pools = queues.memPools(workType);

    pools.uploadPool->beginUsage();
    pools.tablePool->beginUsage();

    if (m_workBundle.totalUploadBufferSize)
        m_uploadMemBlock = pools.uploadPool->allocUploadBlock(m_workBundle.totalUploadBufferSize);

    if ((m_workBundle.totalTableSize + m_workBundle.totalConstantBuffers) > 0)
    {
        Dx12GpuDescriptorTable table = pools.tablePool->allocateTable(m_workBundle.totalTableSize + m_workBundle.totalConstantBuffers);
        m_srvUavTable = table;
        table.advance(m_workBundle.totalTableSize);
        m_cbvTable = table;
    }

    uploadAllTables();

    std::vector<Dx12List> lists;
    std::vector<ID3D12CommandList*> dx12Lists;
    for (int i = 0; i < commandListsCount; ++i)
    {
        lists.emplace_back();
        Dx12List list = lists.back();
        queues.allocate(workType, list);
        dx12Lists.push_back(list.list);

        buildCommandList(i, commandLists[i], *list.list);
    }

   // cmdQueue.ExecuteCommandLists(dx12Lists.size(), dx12Lists.data());

    UINT64 fenceVal = queues.signalFence(workType);
    for (auto l : lists)
        queues.deallocate(l, fenceVal);

    pools.tablePool->endUsage();
    pools.uploadPool->endUsage();
}

}
}
