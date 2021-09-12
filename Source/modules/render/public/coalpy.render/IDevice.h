#pragma once

#include <coalpy.core/Os.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.render/CommandDefs.h>
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

    virtual TextureResult createTexture(const TextureDesc& config) = 0;
    virtual BufferResult  createBuffer (const BufferDesc& config) = 0;
    virtual SamplerResult createSampler (const SamplerDesc& config) = 0;
    virtual InResourceTableResult createInResourceTable  (const ResourceTableDesc& config) = 0;
    virtual OutResourceTableResult  createOutResourceTable (const ResourceTableDesc& config) = 0;
    virtual SamplerTableResult      createSamplerTable (const ResourceTableDesc& config) = 0;

    virtual TextureResult recreateTexture(Texture handle, const TextureDesc& config) = 0;

    virtual void release(ResourceHandle resource) = 0;
    virtual void release(ResourceTable table) = 0;
    virtual void release(WorkHandle handle) = 0;

    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) = 0;
    virtual ScheduleStatus schedule(CommandList** commandLists, int listCounts, ScheduleFlags flags = ScheduleFlags_None) = 0;
    virtual WaitStatus waitOnCpu(WorkHandle bundle, int milliseconds = 0) = 0;
    virtual DownloadStatus getDownloadStatus(WorkHandle workHandle, ResourceHandle handle, int mipLevel = 0, int arraySlice = 0) = 0;
    virtual void getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo) = 0;

    virtual IShaderDb* db() = 0;
};


}
}
