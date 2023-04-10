#pragma once

#include <coalpy.render/Resources.h>
#include <vulkan/vulkan.h>

namespace coalpy
{
namespace render
{

class VulkanDevice;

class VulkanMarkerCollector
{
public:
    enum 
    {
        TimestampByteSize = sizeof(uint64_t)
    };

    explicit VulkanMarkerCollector(VulkanDevice& device)
    : m_device(device)
    {
    }

    ~VulkanMarkerCollector();

    bool isActive() const { return m_active; }
    void beginCollection(int byteCount);
    MarkerResults endCollection();

    void beginMarker(VkCommandBuffer cmdList, const char* markerName);
    void endMarker(VkCommandBuffer cmdList);

private:
    int timestampLeft() const
    {
        return m_timestampCount - m_nextTimestampIndex;
    }

    VulkanDevice& m_device;
    int m_queryHeapBytes = 0;
    int m_timestampCount = 0;
    int m_nextTimestampIndex = 0;
    int m_currentMarker = -1;
    std::vector<MarkerTimestamp> m_markerTimestamps;
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
    Buffer m_timestampBuffer;
    bool m_active = false;
};

}
}
