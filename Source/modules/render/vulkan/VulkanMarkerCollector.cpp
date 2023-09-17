#include <Config.h>
#if ENABLE_VULKAN

#include "VulkanDevice.h"
#include "VulkanMarkerCollector.h"
#include "VulkanQueues.h"
#include "VulkanResources.h"
#include "VulkanGc.h"
#include "VulkanFencePool.h"
#include "VulkanBarriers.h"
#include "Config.h"

namespace coalpy
{
namespace render
{

VulkanMarkerCollector::~VulkanMarkerCollector()
{
    if (m_queryPool != VK_NULL_HANDLE)
    {
        m_device.gc().deferRelease(m_queryPool);
        m_device.release(m_timestampBuffer);
    }
}

void VulkanMarkerCollector::beginCollection(int byteCount)
{
    uint32_t queryCount = (uint32_t)(byteCount + TimestampByteSize - 1) / TimestampByteSize;
    if (byteCount > m_queryHeapBytes)
    {
        if (m_queryPool != VK_NULL_HANDLE)
        {
            m_device.gc().deferRelease(m_queryPool);
            m_device.release(m_timestampBuffer);
            m_queryPool = VK_NULL_HANDLE;
            m_timestampBuffer = Buffer();
        }

        VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr };
        createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount = queryCount;
        VK_OK(vkCreateQueryPool(m_device.vkDevice(), &createInfo, nullptr, &m_queryPool));
        m_queryHeapBytes = byteCount;

        BufferDesc bufferDesc;
        bufferDesc.format = Format::R32_UINT;
        bufferDesc.elementCount = createInfo.queryCount * (TimestampByteSize / sizeof(unsigned));
        auto bufferResult = m_device.createBuffer(bufferDesc);
        CPY_ASSERT(bufferResult.success());
        m_timestampBuffer = bufferResult.object;
        m_timestampCount = createInfo.queryCount;
    }

    
    VulkanQueues& queues = m_device.queues();
    VulkanFencePool& fencePool = m_device.fencePool();

    VulkanList list;
    auto workType = WorkType::Graphics;
    queues.allocate(workType, list);
    VulkanFenceHandle fenceValue = fencePool.allocate();

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(list.list, &beginInfo);
    vkCmdResetQueryPool(list.list, m_queryPool, 0u, queryCount);
    vkEndCommandBuffer(list.list);

    VkQueue queue = queues.cmdQueue(WorkType::Graphics);
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &list.list;    
    VK_OK(vkQueueSubmit(queue, 1u, &submitInfo, fencePool.get(fenceValue)));
    queues.deallocate(list, fenceValue);
    fencePool.free(fenceValue);

    m_markerTimestamps.clear();
    m_currentMarker = -1;
    m_nextTimestampIndex = 0;
    m_active = true;
}

MarkerResults VulkanMarkerCollector::endCollection()
{
    MarkerResults results = {};
    results.timestampBuffer = BufferResult { ResourceResult::InvalidHandle };

    CPY_ASSERT(m_active);
    if (!m_active || m_queryPool == VK_NULL_HANDLE)
        return results;

    VulkanQueues& queues = m_device.queues();
    VulkanFencePool& fencePool = m_device.fencePool();

    VulkanList list;
    auto workType = WorkType::Graphics;
    queues.allocate(workType, list);
    VulkanFenceHandle fenceValue = fencePool.allocate();

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(list.list, &beginInfo);

    BarrierRequest barrier = { m_timestampBuffer, ResourceGpuState::CopyDst };
    inlineApplyBarriers(m_device, &barrier, 1, list.list);

    VulkanResource& timestapmResource = m_device.resources().unsafeGetResource(m_timestampBuffer);

    vkCmdCopyQueryPoolResults(list.list, m_queryPool, 0u, (uint32_t)m_nextTimestampIndex, timestapmResource.bufferData.vkBuffer, 0u, TimestampByteSize, VK_QUERY_RESULT_64_BIT);
    vkCmdResetQueryPool(list.list, m_queryPool, 0u, (uint32_t)m_nextTimestampIndex);
    vkEndCommandBuffer(list.list);

    VkQueue queue = queues.cmdQueue(WorkType::Graphics);
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &list.list;    
    VK_OK(vkQueueSubmit(queue, 1u, &submitInfo, fencePool.get(fenceValue)));
    
    queues.deallocate(list, fenceValue);
    fencePool.free(fenceValue);

    results.timestampBuffer = BufferResult { ResourceResult::Ok, m_timestampBuffer };
    results.markers = m_markerTimestamps.data();
    results.markerCount = (int)m_markerTimestamps.size();
    results.timestampFrequency = (uint64_t)(1000000000.0f / m_device.vkPhysicalDeviceProps().limits.timestampPeriod);
    m_active = false;
    return results;
}

void VulkanMarkerCollector::beginMarker(VkCommandBuffer cmdList, const char* markerName)
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
    vkCmdWriteTimestamp(cmdList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_queryPool, marker.beginTimestampIndex);
}

void VulkanMarkerCollector::endMarker(VkCommandBuffer cmdList)
{
    if (timestampLeft() < 2 || m_currentMarker == -1)
        return;

    MarkerTimestamp& marker = m_markerTimestamps[m_currentMarker];
    vkCmdWriteTimestamp(cmdList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_queryPool, marker.endTimestampIndex);
    m_currentMarker = marker.parentMarkerIndex;
}

}
}

#endif // ENABLE_VULKAN