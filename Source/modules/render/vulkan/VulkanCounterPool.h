#pragma once

#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <vulkan/vulkan.h>
#include <shared_mutex>

namespace coalpy
{
namespace render
{

class VulkanDevice;
struct VulkanCounterHandle : public GenericHandle<int> {};
class VulkanCounterPool
{
public:
    enum
    {
        MaxCounters = 64
    };

    VulkanCounterPool(VulkanDevice& device);
    ~VulkanCounterPool();

    VkBuffer resource() const { return m_resource; }

    VulkanCounterHandle allocate();
    void free(VulkanCounterHandle handle);
    int counterOffset(VulkanCounterHandle handle) const;

private:
    mutable std::shared_mutex m_mutex;

    struct CounterSlot
    {
        int offset = 0;
    };

    VulkanDevice& m_device;
    HandleContainer<VulkanCounterHandle, CounterSlot, (int)MaxCounters> m_counters;
    VkBuffer m_resource = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};

}
}
