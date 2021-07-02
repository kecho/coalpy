#include "WorkBundleDb.h"
#include <coalpy.core/Assert.h>

namespace coalpy
{

namespace render
{

ScheduleStatus WorkBundleDb::build(CommandList** lists, int listCount)
{
    WorkHandle handle;
    WorkBundle newBundle;

    //todo build the bundle here
    {
        std::unique_lock lock(m_workMutex);
        m_works.allocate(handle);
    }

    return ScheduleStatus { handle, ScheduleErrorType::Ok, "" };
}

void WorkBundleDb::release(WorkHandle handle)
{
    std::unique_lock lock(m_workMutex);
    CPY_ASSERT(handle.valid());
    CPY_ASSERT(m_works.contains(handle));
    if (!m_works.contains(handle))
        return;

    m_works.free(handle);
}

void WorkBundleDb::registerTable(ResourceTable table, const ResourceHandle* handles, int handleCounts, bool isUav)
{
    auto& newInfo = m_tables[table];
    newInfo.isUav = isUav;
    newInfo.resources.assign(handles, handles + handleCounts);
}

void WorkBundleDb::unregisterTable(ResourceTable table)
{
    m_tables.erase(table);
}

void WorkBundleDb::registerResource(ResourceHandle handle, ResourceGpuState initialState)
{
    auto& resInfo = m_resources[handle];
    resInfo.gpuState = initialState;
}

void WorkBundleDb::unregisterResource(ResourceHandle handle)
{
    m_resources.erase(handle);
}

}

}
