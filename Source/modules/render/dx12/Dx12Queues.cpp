#include <Config.h>
#if ENABLE_DX12

#include "Dx12Queues.h"
#include "Dx12Device.h"
#include <D3D12.h>

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
    }
}

Dx12Queues::~Dx12Queues()
{
    for (auto& q : m_containers)
    {
        if (q.queue)
            q.queue->Release();
    }
}

}
}

#endif
