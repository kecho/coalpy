#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <coalpy.core/SmartPtr.h>
#include <atomic>
#include <d3d12.h>

namespace coalpy
{
namespace render
{

class Dx12Fence;

//Async GPU garbage collector
class Dx12Gc
{
public:
    Dx12Gc(int frequencyMs, Dx12Fence& fence);
    ~Dx12Gc();

    void start();
    void stop();
    void deferRelease(ID3D12Pageable& object);
    void flush();

private:
    void gatherGarbage(int quota = -1);
    void flushDelete(bool blockCpu = false);
    Dx12Fence& m_fence;

    int m_frequency;
    std::atomic<bool> m_active;

    std::mutex m_gcMutex;
    std::thread m_thread;

    struct ResourceGarbage
    {
        UINT64 fenceValue = {};
        SmartPtr<ID3D12Pageable> object;
    };

    std::queue<SmartPtr<ID3D12Pageable>> m_pendingDeletion;
    std::vector<ResourceGarbage> m_garbage;
};

}
}
