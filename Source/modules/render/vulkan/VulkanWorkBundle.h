#pragma once
#include <stdint.h>

namespace coalpy
{
namespace render
{

class CommandList;
class VulkanDevice;
struct WorkBundle;

class VulkanWorkBundle
{
public:
    VulkanWorkBundle(VulkanDevice& device) : m_device(device) {}
    bool load(const WorkBundle& workBundle);
    uint64_t execute(CommandList** commandLists, int commandListsCount);
private:
    VulkanDevice& m_device;
};

}
}
