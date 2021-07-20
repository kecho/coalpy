#include <Config.h>

#if ENABLE_DX12

#include "Dx12Fence.h"
#include "Dx12Device.h"
#include <D3D12.h>

namespace coalpy
{
namespace render
{

Dx12Fence::Dx12Fence(Dx12Device& device, ID3D12CommandQueue& ownerQueue)
: m_fenceValue(0ull)
, m_device(device)
, m_ownerQueue(ownerQueue)
{
    DX_OK(device.device().CreateFence(0, D3D12_FENCE_FLAG_NONE, DX_RET(m_fence)));
    m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_ownerQueue.AddRef(); 
}

Dx12Fence::~Dx12Fence()
{
    m_ownerQueue.Release();
    if (m_fence)
        m_fence->Release();
    CloseHandle(m_event);
}

UINT64 Dx12Fence::signal()
{
    std::unique_lock lock(m_fenceMutex);
    internalSignal(m_fenceValue + 1);
    return m_fenceValue;
}

UINT64 Dx12Fence::completedValue()
{
    return m_fence->GetCompletedValue();
}

void Dx12Fence::signal(UINT64 value)
{
    std::unique_lock lock(m_fenceMutex);
    internalSignal(value);
}

void Dx12Fence::internalSignal(UINT64 value)
{
    m_fenceValue = value;
    DX_OK(m_ownerQueue.Signal(m_fence, value));
}

bool Dx12Fence::isComplete(UINT64 valueToCmp)
{
    return (m_fence->GetCompletedValue() >= valueToCmp);
}

void Dx12Fence::waitOnCpu(UINT64 valueToWait, DWORD timeMs)
{
    if (m_fence->GetCompletedValue() < valueToWait)
    {
        DX_OK(m_fence->SetEventOnCompletion(valueToWait, m_event));
        WaitForSingleObject(m_event, timeMs);
    }
}

void Dx12Fence::waitOnGpu(UINT64 valueToWait, ID3D12CommandQueue* externalQueue)
{
    auto* targetQueue = externalQueue != nullptr ? externalQueue : &m_ownerQueue;
    DX_OK(targetQueue->Wait(m_fence, valueToWait));
}

}
}

#endif
