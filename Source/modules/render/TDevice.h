#pragma once

#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandDefs.h>
#include "WorkBundleDb.h"

namespace coalpy
{

class IShaderDb;

namespace render
{

template<class PlatDevice>
class TDevice : public IDevice
{
public:
    TDevice(const DeviceConfig& config);
    virtual ~TDevice();

    virtual const DeviceConfig& config() const override { return m_config; }
    virtual ScheduleStatus schedule(CommandList** commandLists, int listCounts, ScheduleFlags flags) override;
    virtual void release(WorkHandle handle) override;

protected:
    IShaderDb& m_db;
    DeviceConfig m_config;
    WorkBundleDb m_workDb;
};

template<class PlatDevice>
TDevice<PlatDevice>::TDevice(const DeviceConfig& config)
: m_config(config), m_db(*config.shaderDb)
{
}

template<class PlatDevice>
TDevice<PlatDevice>::~TDevice()
{
}

template<class PlatDevice>
ScheduleStatus TDevice<PlatDevice>::schedule(CommandList** commandLists, int listCounts, ScheduleFlags flags)
{
    ScheduleStatus status = m_workDb.build(commandLists, listCounts);
    if ((flags & ScheduleFlags_GetWorkHandle) == 0 && status.success() && status.workHandle.valid())
    {
        release(status.workHandle);
        status.workHandle = WorkHandle();
    }

    return status;
}

template<class PlatDevice>
void TDevice<PlatDevice>::release(WorkHandle handle)
{
    m_workDb.release(handle);
}

}

}
