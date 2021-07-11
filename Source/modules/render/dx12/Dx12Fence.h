#pragma once

#include <windows.h>

struct ID3D12Fence;
struct ID3D12CommandQueue;

namespace coalpy
{
namespace render
{

class Dx12Device;

class Dx12Fence 
{
public:
    Dx12Fence(Dx12Device& device, ID3D12CommandQueue& ownerQueue);
    ~Dx12Fence();

    bool isComplete(UINT64 valueToCmp);
    UINT64 value() const { return m_fenceValue; }
    UINT64 signal();
    void signal(UINT64 value);
    void waitOnCpu(UINT64 valueToWait, DWORD timeMs = INFINITE);
    void waitOnGpu(UINT64 valueToWait, ID3D12CommandQueue* externalQueue = nullptr);
    ID3D12Fence& fence() { return *m_fence; }

private:
    Dx12Device& m_device;
    ID3D12CommandQueue& m_ownerQueue;
    ID3D12Fence* m_fence;
    HANDLE m_event;
    UINT64 m_fenceValue;
};

}
}
