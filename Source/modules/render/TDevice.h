#pragma once

#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandDefs.h>
#include <coalpy.core/Assert.h>
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
    //step 1, build the work layout for barriers and tmp resources
    ScheduleStatus status = m_workDb.build(commandLists, listCounts);
    if (!status.success())
        return status;

    //step 2, submit to hardware queues
    CPY_ASSERT(status.workHandle.valid());
    auto& platDevice = *((PlatDevice*)this);
    status = platDevice.internalSchedule(commandLists, listCounts, status.workHandle);

    //step 3, error handle, deallocate and return if hardware scheduling failed.
    if (!status.success())
    {
        if (status.workHandle.valid())
            release(status.workHandle);
        status.workHandle = WorkHandle();
        return status;
    }

    //step 4, write state and if fails, send error back.
    if (!m_workDb.writeResourceStates(status.workHandle))
    {
        if (status.workHandle.valid())
            release(status.workHandle);

        WorkHandle invalidHandle;
        return ScheduleStatus { invalidHandle, ScheduleErrorType::CommitResourceStateFail, "Failed writing resource state after processing command lists." };
    }

    //step 5, if successfull and did not request the work handle, then just deallocate
    if ((flags & ScheduleFlags_GetWorkHandle) == 0 && status.success() && status.workHandle.valid())
    {
        release(status.workHandle);
        status.workHandle = WorkHandle();
    }

    //step 6, return everything
    return status;
}

template<class PlatDevice>
void TDevice<PlatDevice>::release(WorkHandle handle)
{
    m_workDb.release(handle);
}

}

}
