#pragma once

#include "WorkBundleDb.h"
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/GenericHandle.h>
#include <vector>
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

    VulkanEventHandle allocate(CommandLocation location, VkPipelineStageFlags flags, bool& isNew);
    VulkanEventHandle find(CommandLocation location) const;
    VkEvent getEvent(VulkanEventHandle handle) const { return m_records[handle].event; }
    VkPipelineStageFlags getFlags(VulkanEventHandle handle) const { return m_records[handle].flags; }
    void release(VulkanEventHandle handle);

private:
    struct EventRecord
    {
        bool allocated = false;
        CommandLocation location;
        VkPipelineStageFlags flags;
        VkEvent event = {};
    };

    std::vector<VkEvent> m_allocations;
    HandleContainer<VulkanEventHandle, EventRecord> m_records;
    std::unordered_map<CommandLocation, VulkanEventHandle, CommandLocationHasher> m_lookups;
    VulkanDevice& m_device;
};

}
}
