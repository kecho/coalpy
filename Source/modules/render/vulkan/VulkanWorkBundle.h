#pragma once
#include <stdint.h>
#include "WorkBundleDb.h"
#include "VulkanGpuMemPools.h"

namespace coalpy
{
namespace render
{

class CommandList;
class VulkanDevice;

class VulkanWorkBundle
{
public:
    VulkanWorkBundle(VulkanDevice& device) : m_device(device) {}
    bool load(const WorkBundle& workBundle);
    VulkanFenceHandle execute(CommandList** commandLists, int commandListsCount);
private:
    WorkBundle m_workBundle;
    VulkanDevice& m_device;
    VulkanGpuMemoryBlock m_uploadMemBlock;
};

}
}
