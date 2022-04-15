#pragma once

#ifndef INCLUDED_T_DEVICE_H
#include <TDevice.h>
#endif

#include <coalpy.render/Resources.h>
#include <unordered_map>
#include <string>

namespace coalpy
{

class VkShaderDb;

namespace render
{

class VkDevice : public TDevice<VkDevice>
{
public:
    VkDevice(const DeviceConfig& config);
    virtual ~VkDevice();

    static void enumerate(std::vector<DeviceInfo>& outputList);

    virtual TextureResult createTexture(const TextureDesc& desc) override;
    virtual TextureResult recreateTexture(Texture texture, const TextureDesc& desc) override;
    virtual BufferResult  createBuffer (const BufferDesc& config) override;
    virtual SamplerResult createSampler (const SamplerDesc& config) override;
    virtual InResourceTableResult createInResourceTable  (const ResourceTableDesc& config) override;
    virtual OutResourceTableResult createOutResourceTable (const ResourceTableDesc& config) override;
    virtual SamplerTableResult  createSamplerTable (const ResourceTableDesc& config) override;
    virtual void getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo) override;
    virtual WaitStatus waitOnCpu(WorkHandle handle, int milliseconds = 0) override;
    virtual DownloadStatus getDownloadStatus(WorkHandle bundle, ResourceHandle handle, int mipLevel, int arraySlice) override;
    virtual void release(ResourceHandle resource) override;
    virtual void release(ResourceTable table) override;
    virtual const DeviceInfo& info() const override { return m_info; }
    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) override;
    virtual IShaderDb* db() override { return (IShaderDb*)m_shaderDb; }
    void internalReleaseWorkHandle(WorkHandle handle);
    ScheduleStatus internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle); 

private:
    DeviceInfo m_info;
    VkShaderDb* m_shaderDb;
};


}

}
