#pragma once

#include <coalpy.core/Os.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.render/CommandBundles.h>
#include <string>
#include <vector>

namespace coalpy
{

class IShaderDb;

namespace render
{

class CommandList;

enum class DevicePlat
{
    Dx12
};

struct DeviceConfig
{
    DevicePlat platform = DevicePlat::Dx12;
    ModuleOsHandle moduleHandle = nullptr;
    IShaderDb* shaderDb = nullptr;
    int index = -1;
};

struct DeviceInfo
{
    int valid;
    int index;
    std::string name;
};

class IDevice
{
public:
	virtual ~IDevice() {}
    static void enumerate(DevicePlat platform, std::vector<DeviceInfo>& outputList);
    static IDevice * create(const DeviceConfig& config);
    virtual const DeviceConfig& config() const = 0;
    virtual const DeviceInfo& info() const = 0;

    virtual Texture createTexture(const TextureDesc& config) = 0;
    virtual Buffer  createBuffer (const BufferDesc& config) = 0;
    virtual InResourceTable   createInResourceTable  (const ResourceTableDesc& config) = 0;
    virtual OutResourceTable  createOutResourceTable (const ResourceTableDesc& config) = 0;

    virtual void release(ResourceHandle resource) = 0;
    virtual void release(ResourceTable table) = 0;
    virtual void release(CommandListBundle bundle) = 0;

    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) = 0;
    virtual CommandBundleCompileResult compile(CommandList* commandLists, int listCounts) = 0;
    virtual ScheduleStatus   schedule(CommandListBundle bundle, ScheduleFlags flags = ScheduleFlags_ReleaseOnSchedule) = 0;
    virtual WaitBundleStatus waitOnCpu(CommandListBundle bundle, int milliseconds = -1) = 0;
    virtual DownloadStatus getDownloadStatus(CommandListBundle bundle, ResourceHandle handle) = 0;
};


}
}
