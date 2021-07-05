#include "Dx12WorkBundle.h"
#include "Dx12Device.h"
#include "Dx12Resources.h"
#include "Dx12Queues.h"

namespace coalpy
{
namespace render
{

bool Dx12WorkBundle::load(const WorkBundle& workBundle)
{
    m_processedLists = workBundle.processedLists;
    for (auto& it : workBundle.states)
        m_states[it.first] = getDx12GpuState(it.second.state);

    return true;
}

void Dx12WorkBundle::execute(CommandList** commandLists, int commandListsCount)
{
    WorkType workType = WorkType::Graphics;
    auto& queues = m_device.queues();
    ID3D12CommandQueue& cmdQueue = queues.cmdQueue(workType);

    std::vector<Dx12List> lists;
    std::vector<ID3D12CommandList*> dx12Lists;
    for (int i = 0; i < commandListsCount; ++i)
    {
        lists.emplace_back();
        Dx12List list = lists.back();
        queues.allocate(workType, list);
        dx12Lists.push_back(list.list);
    }

   // cmdQueue.ExecuteCommandLists(dx12Lists.size(), dx12Lists.data());

    UINT64 fenceVal = queues.signalFence(workType);
    for (auto l : lists)
        queues.deallocate(l, fenceVal);
}

}
}
