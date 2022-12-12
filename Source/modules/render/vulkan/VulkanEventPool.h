#pragma once

#include "WorkBundleDb.h"
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace coalpy
{
namespace render
{
class VulkanDevice;

class VulkanEventPool
{
public:
    VulkanEventPool(VulkanDevice& device)
    : m_device(device)
    {
    }

    ~VulkanEventPool();

    VkEvent allocate(CommandLocation location, bool& isNew);
    void release(CommandLocation location);

private:
    struct EventRecord
    {
        VkEvent event = {};
        int refCount = 0;
    };

    std::vector<EventRecord> m_records;

    std::unordered_map<CommandLocation, int, CommandLocationHasher> m_lookups;
    std::vector<int> m_freeEvents;
    VulkanDevice& m_device;
};

}
}
