#pragma once

#include "WorkBundleDb.h"
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/GenericHandle.h>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace coalpy
{
namespace render
{
class VulkanDevice;
struct VulkanEventHandle : public GenericHandle<unsigned int> {};

class VulkanEventPool
{
public:
    VulkanEventPool(VulkanDevice& device)
    : m_device(device)
    {
    }

    ~VulkanEventPool();

    VulkanEventHandle allocate(CommandLocation location, bool& isNew);
    VulkanEventHandle find(CommandLocation location) const;
    VkEvent getEvent(VulkanEventHandle handle) const { return m_records[handle].event; }
    void release(VulkanEventHandle location);

private:
    struct EventRecord
    {
        bool allocated = false;
        CommandLocation location;
        VkEvent event = {};
    };

    HandleContainer<VulkanEventHandle, EventRecord> m_records;
    std::unordered_map<CommandLocation, VulkanEventHandle, CommandLocationHasher> m_lookups;
    VulkanDevice& m_device;
};

}
}
