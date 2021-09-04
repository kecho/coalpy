#include "Dx12Gc.h"
#include "Dx12Fence.h"
#include <chrono>

namespace coalpy
{
namespace render
{

Dx12Gc::Dx12Gc(int frequencyMs, Dx12CounterPool& counterPool, Dx12Fence& fence)
: m_frequency(frequencyMs), m_counterPool(counterPool), m_fence(fence), m_active(false)
{
}

Dx12Gc::~Dx12Gc()
{
    if (m_active)
        stop();

    gatherGarbage();
    flushDelete(true /*wait on GPU by blocking CPU*/);
}

void Dx12Gc::deferRelease(ID3D12Pageable& resource)
{
    std::unique_lock lock(m_gcMutex);
    Object object { &resource, Dx12CounterHandle() };
    m_pendingDeletion.push(object);
}

void Dx12Gc::start()
{
    m_active = true;
    m_thread = std::thread([this]()
    {
        while (m_active)
        {
            gatherGarbage(128);
            flushDelete(false /*poll on GPU*/);

            std::this_thread::sleep_for(std::chrono::milliseconds(m_frequency));
        }
    });
}

void Dx12Gc::gatherGarbage(int quota)
{
    std::vector<Object> tmpObjects;
    
    {
        std::unique_lock lock(m_gcMutex);
        while (!m_pendingDeletion.empty() && (quota != 0))
        {
            Object obj = m_pendingDeletion.front();
            m_pendingDeletion.pop();
            tmpObjects.push_back(obj);
            --quota;
        }
    }
    
    if (!tmpObjects.empty())
    {
        UINT64 nextValue = m_fence.signal();
        for (auto& obj : tmpObjects)
        {
            ResourceGarbage g { nextValue, obj };
            m_garbage.push_back(g);
        }
    }
}

void Dx12Gc::flushDelete(bool waitOnCpu)
{
    if (m_garbage.empty())
        return;

    int deleted = 0;
    for (auto& g : m_garbage)
    {
        if (waitOnCpu)
            m_fence.waitOnCpu(g.fenceValue);

        bool performDelete = m_fence.isComplete(g.fenceValue);
        if (performDelete)
        {
            if (g.object.counterHandle.valid())
                m_counterPool.free(g.object.counterHandle);

            g.object = Object();
            g = m_garbage.back();

            m_garbage.back() = ResourceGarbage();
            ++deleted;
        }
    }
    
    if (deleted == (int)m_garbage.size())
        m_garbage.clear();
    else
        m_garbage.resize(m_garbage.size() - deleted);
}

void Dx12Gc::stop()
{
    m_active = false;
    m_thread.join();
}

}
}
