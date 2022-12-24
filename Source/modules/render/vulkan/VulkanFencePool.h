#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/GenericHandle.h>
#include <vector>

namespace coalpy
{
namespace render
{

class VulkanDevice;
struct VulkanFenceHandle : public GenericHandle<unsigned int> {};

class VulkanFencePool
{
public:
    VulkanFencePool(VulkanDevice& device);
    ~VulkanFencePool();

    VulkanFenceHandle allocate();
    void free(VulkanFenceHandle handle);
    VkFence get(VulkanFenceHandle handle) { return m_fences[handle].fence; }
    void addRef(VulkanFenceHandle handle);
    void updateState(VulkanFenceHandle handle);
    void waitOnCpu(VulkanFenceHandle handle);
    bool isSignaled(VulkanFenceHandle handle);

private:
    struct FenceState
    {
        VkFence fence;
        int refCount = 0;
        bool allocated = false;
        bool isSignaled = false;
    };

    HandleContainer<VulkanFenceHandle, FenceState> m_fences;
    VulkanDevice& m_device;
};

}
}
