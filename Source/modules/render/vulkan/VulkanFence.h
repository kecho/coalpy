#pragma once

#include <Config.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <atomic>
#include <shared_mutex>

namespace coalpy
{
namespace render
{

class VulkanDevice;

class VulkanFence
{
public:
    VulkanFence(VulkanDevice& device, VkQueue ownerQueue);
    ~VulkanFence();

    bool isComplete(uint64_t valueToCmp);
    uint64_t completedValue();
    uint64_t nextValue() const { return m_fenceValue; }
    uint64_t signal();
    void signal(uint64_t value);
    void waitOnCpu(uint64_t valueToWait, uint64_t timeMs = -1);
    void waitOnGpu(uint64_t valueToWait, VkQueue queue);
    VkSemaphore& semaphore() { return m_semaphore; }

private:
    void internalSignal(uint64_t value);
    VulkanDevice& m_device;
    VkQueue m_ownerQueue;
    VkSemaphore m_semaphore;
    uint64_t m_fenceValue;
    std::shared_mutex m_fenceMutex;
};

}
}
