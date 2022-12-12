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
    uint64_t timeoutNS = timeMs == -1 ? -1 : (timeMs * 1000000);
    VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, nullptr };
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &m_semaphore;
    waitInfo.pValues = &valueToWait;
    VK_OK(vkWaitSemaphores(m_device.vkDevice(), &waitInfo, timeoutNS));
}

void VulkanFence::waitOnGpu(uint64_t valueToWait, VkQueue queue)
{
    VkSemaphoreSubmitInfo semaphoreSubmitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, nullptr };
    semaphoreSubmitInfo.semaphore = m_semaphore;
    semaphoreSubmitInfo.value = valueToWait;
    semaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2, nullptr };
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &semaphoreSubmitInfo;
    VK_OK(vkQueueSubmit2(queue, 1, &submitInfo, VK_NULL_HANDLE));
}

void VulkanFence::internalSignal(uint64_t value)
{
    VkSemaphoreSubmitInfo semaphoreSubmitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, nullptr };
    semaphoreSubmitInfo.semaphore = m_semaphore;
    semaphoreSubmitInfo.value = value;
    semaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    semaphoreSubmitInfo.deviceIndex = 0;

    VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2, nullptr };
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &semaphoreSubmitInfo;
    VK_OK(vkQueueSubmit2(m_ownerQueue, 1, &submitInfo, nullptr));
}


}
}
