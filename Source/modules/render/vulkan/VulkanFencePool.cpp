#include <Config.h>
#if ENABLE_VULKAN

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
    for (VkFence& fence : m_allocations)
    {
        VK_OK(vkWaitForFences(m_device.vkDevice(), 1, &fence, VK_TRUE, ~0ull));
        vkDestroyFence(m_device.vkDevice(), fence, nullptr);
    }
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
        m_allocations.push_back(fenceState.fence);
        fenceState.allocated = true;
    }

    CPY_ASSERT(fenceState.refCount == 0);
    fenceState.refCount = 1;
    fenceState.isSignaled = false;
    return handle;
}

void VulkanFencePool::addRef(VulkanFenceHandle handle)
{
    FenceState& fenceState = m_fences[handle];
    CPY_ASSERT(fenceState.refCount > 0);
    CPY_ASSERT(fenceState.allocated);
    ++fenceState.refCount;
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

bool VulkanFencePool::waitOnCpu(VulkanFenceHandle handle, uint64_t milliseconds)
{
    FenceState& fenceState = m_fences[handle];
    CPY_ASSERT(fenceState.allocated);
    VkResult result = vkWaitForFences(m_device.vkDevice(), 1, &fenceState.fence, VK_TRUE, milliseconds == ~0ull ? ~0ull : (milliseconds * 1000000));
    CPY_ASSERT_FMT((milliseconds == ~0ull && result == VK_SUCCESS) || (milliseconds != ~0ull), "%s", "illegal timeout on infinite wait time.");

    return result == VK_SUCCESS;
}

void VulkanFencePool::free(VulkanFenceHandle handle)
{
    FenceState& fenceState = m_fences[handle];
    CPY_ASSERT(fenceState.refCount > 0);
    --fenceState.refCount;
    if (fenceState.refCount == 0)
        m_fences.free(handle, false);
}

}
}

#endif // ENABLE_VULKAN