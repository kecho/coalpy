#include <Config.h>
#if ENABLE_DX12

#include "Dx12Queues.h"
#include <D3D12.h>

namespace coalpy
{
namespace render
{

Dx12Queues::Dx12Queues(Dx12Device& device)
: m_device(device)
{
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
