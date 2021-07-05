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
    std::vector<ResourceBarrier> preBarrier;
    std::vector<ResourceBarrier> postBarrier;
};

struct ProcessedList
{
    int listIndex = 0;
    std::vector<CommandInfo> commandSchedule;
};

struct WorkBundle
{
    std::vector<ProcessedList> processedLists;
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

private:
    std::mutex m_workMutex;
    HandleContainer<WorkHandle, WorkBundle> m_works;

    WorkTableInfos m_tables;
    WorkResourceInfos m_resources;
};

}

} 
