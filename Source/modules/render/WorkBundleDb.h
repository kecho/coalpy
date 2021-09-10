#pragma once

#include <coalpy.render/CommandDefs.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/AbiCommands.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.core/HandleContainer.h>
#include <vector>
#include <mutex>
#include <string>
#include <set>
#include <unordered_map>

namespace coalpy
{

namespace render
{

class IDevice;

enum class ResourceGpuState
{
    Default,
    IndirectArgs,
    Srv,
    Uav,
    Cbv,
    Rtv,
    CopySrc,
    CopyDst,
    Present
};

enum class BarrierType
{
    Immediate,
    Begin,
    End
};

struct ResourceBarrier
{
    ResourceHandle resource;
    bool isUav = false; //ignores previous and post states
    ResourceGpuState prevState = ResourceGpuState::Default;
    ResourceGpuState postState = ResourceGpuState::Default;
    BarrierType type = BarrierType::Immediate;
};

struct CommandInfo
{
    MemOffset commandOffset = {};
    int uploadBufferOffset = 0;
    int commandDownloadIndex = -1;
    int constantBufferTableOffset = -1;
    int constantBufferCount = 0;
    ResourceMemoryInfo uploadDestinationMemoryInfo;
    std::vector<ResourceBarrier> preBarrier;
    std::vector<ResourceBarrier> postBarrier;
};

struct TableAllocation
{
    int offset = 0;
    int count = 0;
    bool isSampler = false;
};

struct ProcessedList
{
    int listIndex = 0;
    int computeCommandsCount = 0;
    int downloadCommandsCount = 0;
    std::vector<CommandInfo> commandSchedule;
};

struct WorkResourceState
{
    int listIndex;
    int commandIndex;
    ResourceGpuState state;
};

using ResourceStateMap = std::unordered_map<ResourceHandle, WorkResourceState>;
using TableGpuAllocationMap = std::unordered_map<ResourceTable, TableAllocation>;
using ResourceSet  = std::set<ResourceHandle>;

struct WorkBundle
{
    std::vector<ProcessedList> processedLists;
    ResourceStateMap states;

    int totalTableSize = 0;
    int totalConstantBuffers = 0;
    int totalUploadBufferSize = 0;
    int totalSamplers = 0;

    ResourceSet resourcesToDownload;
    TableGpuAllocationMap tableAllocations;
};

struct WorkTableInfo
{
    bool isUav;
    std::string name;
    std::vector<ResourceHandle> resources;
};

struct WorkResourceInfo
{
    MemFlags memFlags = {};
    ResourceGpuState gpuState = ResourceGpuState::Default;
    Buffer counterBuffer;
    int mipLevels = 1;
    int arraySlices = 1;
};

using WorkTableInfos = std::unordered_map<ResourceTable,  WorkTableInfo>;
using WorkResourceInfos = std::unordered_map<ResourceHandle, WorkResourceInfo>;

class WorkBundleDb
{
public:
    WorkBundleDb(IDevice& device) : m_device(device) {}
    ~WorkBundleDb() {}

    ScheduleStatus build(CommandList** lists, int listCount);
    void release(WorkHandle);

    void registerTable(ResourceTable table, const char* name, const ResourceHandle* handles, int handleCounts, bool isUav);
    void unregisterTable(ResourceTable table);
    void clearAllTables() { m_tables.clear(); }

    void registerResource(
        ResourceHandle handle,
        MemFlags flags,
        ResourceGpuState initialState,
        int mipLevels, int arraySlices,
        Buffer counterBuffer = Buffer());

    void unregisterResource(ResourceHandle handle);
    void clearAllResources() { m_resources.clear(); }

    bool writeResourceStates(WorkHandle handle);

    void lock() { m_workMutex.lock(); }
    WorkBundle& unsafeGetWorkBundle(WorkHandle handle) { return m_works[handle]; }
    WorkResourceInfos& resourceInfos() { return m_resources; }
    WorkTableInfos& tableInfos() { return m_tables; }
    void unlock() { m_workMutex.unlock(); }

private:
    std::mutex m_workMutex;

    IDevice& m_device;
    HandleContainer<WorkHandle, WorkBundle> m_works;

    WorkTableInfos m_tables;
    WorkResourceInfos m_resources;
};

}

} 
