#pragma once

#include <coalpy.render/CommandDefs.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/AbiCommands.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.core/HandleContainer.h>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace coalpy
{

namespace render
{

enum class ResourceGpuState
{
    Default,
    IndirectArgs,
    Srv,
    Uav,
    Cbv,
    CopySrc,
    CopyDst
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
    ResourceGpuState prevState = ResourceGpuState::Default;
    ResourceGpuState postState = ResourceGpuState::Default;
    BarrierType type = BarrierType::Immediate;
};

struct CommandInfo
{
    MemOffset commandOffset = {};
    int uploadBufferOffset = 0;
    std::vector<ResourceBarrier> preBarrier;
    std::vector<ResourceBarrier> postBarrier;
};

struct TableAllocation
{
    int offset = 0;
    int count = 0;
};

struct ProcessedList
{
    int listIndex = 0;
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
using ConstantDescriptorOffsetMap = std::unordered_map<ResourceHandle, int>;

struct WorkBundle
{
    std::vector<ProcessedList> processedLists;
    ResourceStateMap states;

    int totalTableSize = 0;
    int totalConstantBuffers = 0;
    int totalUploadBufferSize = 0;
    TableGpuAllocationMap tableAllocations;
    ConstantDescriptorOffsetMap constantDescriptorOffsets;
};

struct WorkTableInfo
{
    bool isUav;
    std::vector<ResourceHandle> resources;
};

struct WorkResourceInfo
{
    ResourceGpuState gpuState = ResourceGpuState::Default;
};

using WorkTableInfos = std::unordered_map<ResourceTable,  WorkTableInfo>;
using WorkResourceInfos = std::unordered_map<ResourceHandle, WorkResourceInfo>;

class WorkBundleDb
{
public:
    WorkBundleDb() {}
    ~WorkBundleDb() {}

    ScheduleStatus build(CommandList** lists, int listCount);
    void release(WorkHandle);

    void registerTable(ResourceTable table, const ResourceHandle* handles, int handleCounts, bool isUav);
    void unregisterTable(ResourceTable table);
    void clearAllTables() { m_tables.clear(); }

    void registerResource(ResourceHandle handle, ResourceGpuState initialState);
    void unregisterResource(ResourceHandle handle);
    void clearAllResources() { m_resources.clear(); }

    bool writeResourceStates(WorkHandle handle);

    void lock() { m_workMutex.lock(); }
    WorkBundle& unsafeGetWorkBundle(WorkHandle handle) { return m_works[handle]; }
    void unlock() { m_workMutex.unlock(); }

private:
    std::mutex m_workMutex;
    HandleContainer<WorkHandle, WorkBundle> m_works;

    WorkTableInfos m_tables;
    WorkResourceInfos m_resources;
};

}

} 
