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
    CurrentState,
    Default,
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
    ResourceGpuState prevState = ResourceGpuState::CurrentState;
    ResourceGpuState postState = ResourceGpuState::CurrentState;
    BarrierType type = BarrierType::Immediate;
    int subresourceIndex = 0;
};

struct BarrierSchedule
{
    MemOffset commandOffset = {};
    std::vector<ResourceBarrier> preBarrier;
    std::vector<ResourceBarrier> postBarrier;
};

struct ProcessedList
{
    int listIndex = 0;
    std::vector<BarrierSchedule> barrierSchedules;
};

struct WorkBundle
{
    std::vector<ProcessedList> processedLists;
};

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

    struct TableInfo
    {
        bool isUav;
        std::vector<ResourceHandle> resources;
    };

    struct ResourceInfo
    {
        ResourceGpuState gpuState = ResourceGpuState::Default;
    };

    std::unordered_map<ResourceTable,  TableInfo>    m_tables;
    std::unordered_map<ResourceHandle, ResourceInfo> m_resources;
};

}

} 
