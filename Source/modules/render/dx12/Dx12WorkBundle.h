#pragma once

#include <coalpy.render/Resources.h>
#include "WorkBundleDb.h"
#include <d3d12.h>
#include <unordered_map>
#include <vector>

namespace coalpy
{
namespace render
{

class CommandList;
class Dx12Queues;
class Dx12Device;

class Dx12WorkBundle
{
public:
    Dx12WorkBundle(Dx12Device& device) : m_device(device) {}
    bool load(const WorkBundle& workBundle);

    void execute(CommandList** commandLists, int commandListsCount);

private:
    Dx12Device& m_device;
    std::vector<ProcessedList> m_processedLists;
    std::unordered_map<ResourceHandle, D3D12_RESOURCE_STATES> m_states;
};

}
}
