#pragma once

#include <coalpy.render/Resources.h>
#include "WorkBundleDb.h"
#include "Dx12Utils.h"
#include "Dx12GpuMemPools.h"
#include "Dx12Queues.h"
#include "Dx12BufferPool.h"
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

struct Dx12ResourceDownloadState
{
    ResourceDownloadKey downloadKey;
    WorkType queueType = WorkType::Graphics;
    UINT64 fenceValue = {};
    Dx12CpuMemBlock memoryBlock;
    ResourceHandle resource;
    size_t rowPitch = 0;
    int width  = 0;
    int height = 0;
    int depth  = 0;
};

using Dx12DownloadResourceMap = std::unordered_map<ResourceDownloadKey, Dx12ResourceDownloadState>;

class Dx12WorkBundle
{
public:
    Dx12WorkBundle(Dx12Device& device) : m_device(device) {}
    bool load(const WorkBundle& workBundle);

    UINT64 execute(CommandList** commandLists, int commandListsCount);

    void getDownloadResourceMap(Dx12DownloadResourceMap& downloadMap);

private:
    void uploadAllTables();
    void applyBarriers(const std::vector<ResourceBarrier>& barriers, ID3D12GraphicsCommandListX& outList);
    void buildCommandList(int listIndex, const CommandList* cmdList, WorkType workType, ID3D12GraphicsCommandListX& outList);
    void buildComputeCmd(const unsigned char* data, const AbiComputeCmd* computeCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList);
    void buildDownloadCmd(const unsigned char* data, const AbiDownloadCmd* downloadCmd,  const CommandInfo& cmdInfo, WorkType workType, ID3D12GraphicsCommandListX& outList);
    void buildCopyCmd(const unsigned char* data, const AbiCopyCmd* copyCmd, ID3D12GraphicsCommandListX& outList);
    void buildUploadCmd(const unsigned char* data, const AbiUploadCmd* uploadCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList);
    void buildCopyAppendConsumeCounter(const unsigned char* data, const AbiCopyAppendConsumeCounter* abiCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList);
    void buildClearAppendConsumeCounter(const unsigned char* data, const AbiClearAppendConsumeCounter* abiCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList);

    Dx12Device& m_device;
    WorkBundle m_workBundle;
    Dx12GpuDescriptorTable m_srvUavTable;
    Dx12GpuDescriptorTable m_cbvTable;
    Dx12GpuDescriptorTable m_samplersTable;
    Dx12GpuMemoryBlock m_uploadMemBlock;
    Dx12GpuMemoryBlock m_constantMemBlock;
    std::unordered_map<ResourceHandle, D3D12_RESOURCE_STATES> m_states;
    std::vector<Dx12ResourceDownloadState> m_downloadStates;
    UINT64 m_currentFenceValue = {};
};

}
}
