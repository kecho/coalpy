#include <Config.h>
#include <coalpy.core/Assert.h>
#include "VulkanEventPool.h"
#include "VulkanDevice.h"

namespace coalpy
{
namespace render
{

VulkanEventPool::~VulkanEventPool()
{
    for (VkEvent& e : m_allocations)
        vkDestroyEvent(m_device.vkDevice(), e, nullptr); 
}

VulkanEventHandle VulkanEventPool::allocate(CommandLocation location, VkPipelineStageFlags flags, bool& isNew)
{
    VulkanEventHandle handle;
    EventRecord& record = m_records.allocate(handle);
    if (!record.allocated)
    {
        isNew = true;
        VkEventCreateInfo createInfo = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, nullptr, VK_EVENT_CREATE_DEVICE_ONLY_BIT };
        VK_OK(vkCreateEvent(m_device.vkDevice(), &createInfo, nullptr, &record.event));
        record.allocated = true;
        m_allocations.push_back(record.event);
    }
    else
        isNew = false;

    record.location = location;
    record.flags = flags;
    m_lookups.insert(std::pair<CommandLocation, VulkanEventHandle>(location, handle));
    return handle;
}

void VulkanEventPool::release(VulkanEventHandle handle)
{
    EventRecord& record = m_records[handle];
    CPY_ASSERT(record.allocated);
    m_lookups.erase(record.location);
    m_records.free(handle, false);
}


VulkanEventHandle VulkanEventPool::find(CommandLocation location) const
{
    auto it = m_lookups.find(location);
    CPY_ASSERT(it != m_lookups.end());
    if (it == m_lookups.end())
        return VulkanEventHandle();

    return it->second; 
}

}
}
