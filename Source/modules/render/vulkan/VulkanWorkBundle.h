#pragma once
#include <stdint.h>
#include <vector>
#include "WorkBundleDb.h"
#include "VulkanQueues.h"
#include "VulkanGpuMemPools.h"

namespace coalpy
{
namespace render
{

class CommandList;
class VulkanDevice;
struct VulkanList;
struct AbiCopyCmd;
struct AbiUploadCmd;

class VulkanWorkBundle
{
public:
    VulkanWorkBundle(VulkanDevice& device) : m_device(device) {}
    bool load(const WorkBundle& workBundle);
    VulkanFenceHandle execute(CommandList** commandLists, int commandListsCount);
private:
    void buildUploadCmd(const unsigned char* data, const AbiUploadCmd* uploadCmd, const CommandInfo& cmdInfo, VulkanList& outList);
    void buildCopyCmd(const unsigned char* data, const AbiCopyCmd* copyCmd, const CommandInfo& cmdInfo, VulkanList& outList);
    void buildCommandList(int listIndex, const CommandList* cmdList, WorkType workType, VulkanList& list, std::vector<VulkanEventHandle>& events);

    WorkBundle m_workBundle;
    VulkanDevice& m_device;
    VulkanGpuMemoryBlock m_uploadMemBlock;
};

}
}
