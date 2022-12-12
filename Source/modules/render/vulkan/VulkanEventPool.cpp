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
    for (auto& record : m_records)
        vkDestroyEvent(m_device.vkDevice(), record.event, nullptr);
}

VkEvent VulkanEventPool::allocate(CommandLocation location, bool& isNew)
{
    VkEvent outEvent = {};
    auto it = m_lookups.find(location);
    if (it == m_lookups.end())
    {
        isNew = true;
        int recordIndex = 0;
        if (m_freeEvents.empty())
        {
            recordIndex = m_records.size();
            m_records.emplace_back(EventRecord { outEvent, 1 });
            VkEventCreateInfo createInfo = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, nullptr, VK_EVENT_CREATE_DEVICE_ONLY_BIT };
            VK_OK(vkCreateEvent(m_device.vkDevice(), &createInfo, nullptr, &outEvent));
        }
        else
        {
            recordIndex = m_freeEvents.back();
            m_freeEvents.pop_back();
            EventRecord& record = m_records[recordIndex];
            CPY_ASSERT(record.refCount == 0);
            record.refCount++;
        }

        m_lookups.insert(std::pair<CommandLocation, int>(location, recordIndex));
    }
    else
    {
        isNew = false;
        EventRecord& record = m_records[it->second];
        ++record.refCount;
        outEvent = record.event;
    }
    return outEvent;
}

void VulkanEventPool::release(CommandLocation location)
{
    auto it = m_lookups.find(location);
    CPY_ASSERT(it != m_lookups.end());
    if (it == m_lookups.end())
        return;

    EventRecord& record = m_records[it->second];
    --record.refCount;
    CPY_ASSERT(record.refCount >= 0);
    if (record.refCount > 0)
        return;

    m_freeEvents.push_back(it->second);
}

}
}
