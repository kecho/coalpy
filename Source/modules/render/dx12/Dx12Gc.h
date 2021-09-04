#pragma once

#include "Dx12CounterPool.h"
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
class Dx12CounterPool;

//Async GPU garbage collector
class Dx12Gc
{
public:
    Dx12Gc(int frequencyMs, Dx12CounterPool& counterPool, Dx12Fence& fence);
    ~Dx12Gc();

    void start();
    void stop();
    void deferRelease(ID3D12Pageable& object, Dx12CounterHandle counterHandle = Dx12CounterHandle());
    void flush();

private:
    void gatherGarbage(int quota = -1);
    void flushDelete(bool blockCpu = false);
    Dx12Fence& m_fence;

    int m_frequency;
    std::atomic<bool> m_active;

    std::mutex m_gcMutex;
    std::thread m_thread;

    struct Object
    {
        SmartPtr<ID3D12Pageable> resource;
        Dx12CounterHandle counterHandle;
    };

    struct ResourceGarbage
    {
        UINT64 fenceValue = {};
        Object object;
    };

    Dx12CounterPool& m_counterPool;
    std::queue<Object> m_pendingDeletion;
    std::vector<ResourceGarbage> m_garbage;
};

}
}
