#include "VulkanQueues.h"
#include "VulkanFence.h"
#include "VulkanDevice.h"


namespace coalpy
{
namespace render
{

VulkanQueues::VulkanQueues(VulkanDevice& device)
: m_device(device)
{
    for (int queueIt = 0u; queueIt < (int)WorkType::Count; ++queueIt)
    {
        QueueContainer& qcontainer = m_containers[queueIt];
        vkGetDeviceQueue(m_device.vkDevice(), m_device.graphicsFamilyQueueIndex(), (uint32_t)queueIt, &qcontainer.queue);
    }
}

VulkanQueues::~VulkanQueues()
{
}

void VulkanQueues::allocate(WorkType workType, VulkanList& outList)
{
}

uint64_t VulkanQueues::currentFenceValue(WorkType workType)
{
    return {};
}

uint64_t VulkanQueues::signalFence(WorkType workType)
{
    return {};
}

void VulkanQueues::deallocate(VulkanList& list, uint64_t fenceValue)
{
}

}
}
