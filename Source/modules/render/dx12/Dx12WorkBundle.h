#pragma once

#include <coalpy.render/Resources.h>
#include "WorkBundleDb.h"
#include "Dx12GpuMemPools.h"
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
    void uploadAllTables();
    void applyBarriers(const std::vector<ResourceBarrier>& barriers, ID3D12GraphicsCommandList6& outList);
    void buildCommandList(int listIndex, const CommandList* cmdList, ID3D12GraphicsCommandList6& outList);

    Dx12Device& m_device;
    WorkBundle m_workBundle;
    Dx12GpuDescriptorTable m_srvUavTable;
    Dx12GpuDescriptorTable m_cbvTable;
    Dx12GpuMemoryBlock m_uploadMemBlock;
    std::unordered_map<ResourceHandle, D3D12_RESOURCE_STATES> m_states;
};

}
}
