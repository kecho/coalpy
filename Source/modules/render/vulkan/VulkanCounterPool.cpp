#include <coalpy.core/Assert.h>
#include <Config.h>
#include "VulkanCounterPool.h"
#include "VulkanDevice.h"

//Alignment for buffer counter
#define D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT 4069 

namespace coalpy
{
namespace render
{

VulkanCounterPool::VulkanCounterPool(VulkanDevice& device)
: m_device(device)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr };
    createInfo.usage = (VkBufferUsageFlags)(VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    createInfo.size = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * MaxCounters;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //not exposed, cant do async in coalpy yet.
    createInfo.queueFamilyIndexCount = 1;
    uint32_t desfaultQueueFam = m_device.graphicsFamilyQueueIndex();
    createInfo.pQueueFamilyIndices = &desfaultQueueFam;
    VK_OK(vkCreateBuffer(m_device.vkDevice(), &createInfo, nullptr, &m_resource));

    VkMemoryRequirements memReqs = {};
    vkGetBufferMemoryRequirements(m_device.vkDevice(), m_resource, &memReqs);

    VkMemoryPropertyFlags memProperties = {};
    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr };
    allocInfo.allocationSize = memReqs.size;
    if (!m_device.findMemoryType(memReqs.memoryTypeBits, memProperties, allocInfo.memoryTypeIndex))
    {
        CPY_ERROR_MSG(false, "Failed to allocate counter pool buffer");
    }
    else
    {
        VK_OK(vkAllocateMemory(m_device.vkDevice(), &allocInfo, nullptr, &m_memory));
        VK_OK(vkBindBufferMemory(m_device.vkDevice(), m_resource, m_memory, 0u));
    }
}

VulkanCounterPool::~VulkanCounterPool()
{
    if (m_resource)
        vkDestroyBuffer(m_device.vkDevice(), m_resource, nullptr);
    if (m_memory)
        vkFreeMemory(m_device.vkDevice(), m_memory, nullptr);
}

VulkanCounterHandle VulkanCounterPool::allocate()
{
    std::unique_lock lock(m_mutex);
    VulkanCounterHandle handle;
    CounterSlot& slot = m_counters.allocate(handle);
    if (handle.valid())
        slot.offset = handle.handleId * D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;

    return handle;
}

int VulkanCounterPool::counterOffset(VulkanCounterHandle handle) const
{
    std::shared_lock lock(m_mutex);
    const bool isValid = handle.valid() && m_counters.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return 0;

    return m_counters[handle].offset;
}

void VulkanCounterPool::free(VulkanCounterHandle handle)
{
    std::unique_lock lock(m_mutex);

    const bool isValid = handle.valid() && m_counters.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;

    m_counters.free(handle);
}


}
}
