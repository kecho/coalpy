#include "Dx12Device.h"
#include "Dx12MarkerCollector.h"
#include "Dx12Queues.h"
#include "Dx12ResourceCollection.h"
#include "Config.h"
#include <d3d12.h>

namespace coalpy
{
namespace render
{

Dx12MarkerCollector::~Dx12MarkerCollector()
{
    if (m_queryHeap != nullptr)
    {
        m_device.deferRelease(*m_queryHeap);
        m_device.release(m_timestampBuffer);
    }
}

void Dx12MarkerCollector::beginCollection(int byteCount)
{
    if (byteCount >= m_queryHeapBytes)
    {
        if (m_queryHeap != nullptr)
        {
            m_device.deferRelease(*m_queryHeap);
            m_device.release(m_timestampBuffer);
            m_queryHeap = nullptr;
            m_timestampBuffer = Buffer();
        }
        
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Count = (byteCount + TimestampByteSize - 1) / TimestampByteSize;
        desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        DX_OK(m_device.device().CreateQueryHeap(&desc, DX_RET(m_queryHeap)));
        m_queryHeapBytes = byteCount;

        BufferDesc bufferDesc;
        bufferDesc.format = Format::R32_UINT;
        bufferDesc.elementCount = desc.Count * (TimestampByteSize / sizeof(unsigned));
        auto bufferResult = m_device.createBuffer(bufferDesc);
        CPY_ASSERT(bufferResult.success());
        m_timestampBuffer = bufferResult.object;
        m_timestampCount = desc.Count;
    }

    m_markerTimestamps.clear();
    m_currentMarker = -1;
    m_nextTimestampIndex = 0;
    m_active = true;
}

MarkerResults Dx12MarkerCollector::endCollection()
{
    MarkerResults results = {};
    results.timestampBuffer = BufferResult { ResourceResult::InvalidHandle };

    CPY_ASSERT(m_active);
    if (!m_active || m_queryHeap == nullptr)
        return results;

    Dx12List dx12List;
    auto workType = WorkType::Graphics;
    m_device.queues().allocate(workType, dx12List);

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    m_device.transitionResourceState(m_timestampBuffer, D3D12_RESOURCE_STATE_COPY_DEST, barriers);
    if (!barriers.empty())
    {
        dx12List.list->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }

    dx12List.list->ResolveQueryData(
        m_queryHeap,
        D3D12_QUERY_TYPE_TIMESTAMP,
        0,
        m_nextTimestampIndex,
        &m_device.resources().unsafeGetResource(m_timestampBuffer).d3dResource(),
        0u);

    dx12List.list->Close();
    //submit andfree
    {
        ID3D12CommandList* cmdList = &(*dx12List.list);
        m_device.queues().cmdQueue(workType).ExecuteCommandLists(1, &cmdList);
        auto fenceVal = m_device.queues().signalFence(workType);
        m_device.queues().deallocate(dx12List, fenceVal);
    }

    DX_OK(m_device.queues().cmdQueue(workType).GetTimestampFrequency(&results.timestampFrequency));

    results.timestampBuffer = BufferResult { ResourceResult::Ok, m_timestampBuffer };
    results.markers = m_markerTimestamps.data();
    results.markerCount = (int)m_markerTimestamps.size();
    m_active = false;
    return results;
}

void Dx12MarkerCollector::beginMarker(ID3D12GraphicsCommandList& cmdList, const char* markerName)
{
    CPY_ASSERT(timestampLeft() >= 2);
    if (timestampLeft() < 2)
        return;

    int parentMarker = m_currentMarker;
    m_currentMarker = m_markerTimestamps.size();
    m_markerTimestamps.emplace_back();
    MarkerTimestamp& marker = m_markerTimestamps.back();
    marker.name = markerName;
    marker.parentMarkerIndex = parentMarker;
    marker.beginTimestampIndex = m_nextTimestampIndex++;
    marker.endTimestampIndex = m_nextTimestampIndex++;
    cmdList.EndQuery(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, marker.beginTimestampIndex);
}

void Dx12MarkerCollector::endMarker(ID3D12GraphicsCommandList& cmdList)
{
    if (timestampLeft() < 2 || m_currentMarker == -1)
        return;

    MarkerTimestamp& marker = m_markerTimestamps[m_currentMarker];
    cmdList.EndQuery(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, marker.endTimestampIndex);
    m_currentMarker = marker.parentMarkerIndex;
}

}
}
