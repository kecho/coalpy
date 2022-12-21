#include <Config.h>
#include "VulkanDevice.h"
#include "VulkanFencePool.h"

namespace coalpy
{
namespace render
{

VulkanFencePool::VulkanFencePool(VulkanDevice& device)
: m_device(device)
{
}

VulkanFencePool::~VulkanFencePool()
{
    m_fences.forEach([this](VulkanFenceHandle handle, FenceState& fenceState)
    {
        waitOnCpu(handle);
        vkDestroyFence(m_device.vkDevice(), fenceState.fence, nullptr);
    });
}

VulkanFenceHandle VulkanFencePool::allocate()
{
    VulkanFenceHandle handle;
    FenceState& fenceState = m_fences.allocate(handle);
        
    if (fenceState.allocated)
    {
        VK_OK(vkResetFences(m_device.vkDevice(), 1, &fenceState.fence));
    }
    else
    {
        VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
        createInfo.flags = 0;
        VK_OK(vkCreateFence(m_device.vkDevice(), &createInfo, nullptr, &fenceState.fence));
        fenceState.allocated = true;
    }
    fenceState.isSignaled = false;
    return handle;
}

void VulkanFencePool::updateState(VulkanFenceHandle handle)
{
    FenceState& fenceState = m_fences[handle];
    CPY_ASSERT(fenceState.allocated);
    VkResult result = vkGetFenceStatus(m_device.vkDevice(), fenceState.fence);
    fenceState.isSignaled = result == VK_SUCCESS;
}

bool VulkanFencePool::isSignaled(VulkanFenceHandle handle)
{
    FenceState& fenceState = m_fences[handle];
    CPY_ASSERT(fenceState.allocated);
    return fenceState.isSignaled;
}

void VulkanFencePool::waitOnCpu(VulkanFenceHandle handle)
{
    FenceState& fenceState = m_fences[handle];
    CPY_ASSERT(fenceState.allocated);
    VK_OK(vkWaitForFences(m_device.vkDevice(), 1, &fenceState.fence, VK_TRUE, ~0ull));
}

void VulkanFencePool::free(VulkanFenceHandle handle)
{
    m_fences.free(handle, false);
}

}
}
