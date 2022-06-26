#pragma once

#include <coalpy.render/Resources.h>

struct ID3D12QueryHeap;
struct ID3D12CommandList;

namespace coalpy
{
namespace render
{

class Dx12Device;

class Dx12MarkerCollector
{
public:
    enum 
    {
        TimestampByteSize = sizeof(uint64_t)
    };

    explicit Dx12MarkerCollector(Dx12Device& device)
    : m_device(device)
    {
    }

    ~Dx12MarkerCollector();

    bool isActive() const { return m_active; }
    void beginCollection(int byteCount);
    MarkerResults endCollection();

    void beginMarker(ID3D12GraphicsCommandList& cmdList, const char* markerName);
    void endMarker(ID3D12GraphicsCommandList& cmdList);

private:
    int timestampLeft() const
    {
        return m_timestampCount - m_nextTimestampIndex;
    }

    Dx12Device& m_device;
    int m_queryHeapBytes = 0;
    int m_timestampCount = 0;
    int m_nextTimestampIndex = 0;
    int m_currentMarker = -1;
    std::vector<MarkerTimestamp> m_markerTimestamps;
    ID3D12QueryHeap* m_queryHeap = nullptr;
    Buffer m_timestampBuffer;
    bool m_active = false;
};

}
}
