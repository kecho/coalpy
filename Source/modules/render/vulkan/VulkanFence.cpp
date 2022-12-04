#include "VulkanFence.h"
#include "VulkanDevice.h"

namespace coalpy
{
namespace render
{

VulkanFence::VulkanFence(VulkanDevice& device, VkQueue ownerQueue)
: m_device(device)
, m_ownerQueue(ownerQueue)
{
    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr };
    createInfo.flags = {};
    VK_OK(vkCreateSemaphore(m_device.vkDevice(), &createInfo, nullptr, &m_semaphore));
}

VulkanFence::~VulkanFence()
{
    vkDestroySemaphore(m_device.vkDevice(), m_semaphore, nullptr);
}

bool VulkanFence::isComplete(uint64_t valueToCmp)
{
    return completedValue() >= valueToCmp;
}

uint64_t VulkanFence::completedValue()
{
    uint64_t outVal = {};
    VK_OK(vkGetSemaphoreCounterValue(m_device.vkDevice(), m_semaphore, &outVal));
    return outVal;
}

uint64_t VulkanFence::signal()
{
    std::unique_lock lock(m_fenceMutex);
    internalSignal(m_fenceValue + 1);
    return m_fenceValue;
}

void VulkanFence::signal(uint64_t value)
{
    std::unique_lock lock(m_fenceMutex);
    internalSignal(value);
}

void VulkanFence::waitOnCpu(uint64_t valueToWait, uint64_t timeMs)
{
}

void VulkanFence::waitOnGpu(uint64_t valueToWait, VkQueue queue)
{
}

void VulkanFence::internalSignal(uint64_t value)
{
}


}
}
