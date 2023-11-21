#pragma once

#ifndef INCLUDED_T_DEVICE_H
#include <TDevice.h>
#endif

#include <coalpy.core/BitMask.h>
#include <coalpy.render/Resources.h>
#include <unordered_map>
#include <string>

@protocol MTLDevice;

namespace coalpy
{

class MetalShaderDb;

namespace render
{

// class MetalDescriptorSetPools;
// class MetalReadbackBufferPool;
class MetalResources;
class MetalQueues;
// class MetalCounterPool;
// class MetalEventPool;
// class MetalFencePool;
// class MetalGc;
// class MetalMarkerCollector;
// struct MetalWorkInformationMap;

class MetalDevice : public TDevice<MetalDevice>
{
public:
    MetalDevice(const DeviceConfig& config);
    virtual ~MetalDevice();

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
    virtual const DeviceRuntimeInfo& runtimeInfo() const override { return m_runtimeInfo; }
    virtual SmartPtr<IDisplay> createDisplay (const DisplayConfig& config) override;
    virtual void removeShaderDb() override { m_shaderDb = nullptr; }
    virtual IShaderDb* db() override { return (IShaderDb*)m_shaderDb; }
    virtual void beginCollectMarkers(int maxQueryBytes) override;
    virtual MarkerResults endCollectMarkers() override;
    void internalReleaseWorkHandle(WorkHandle handle);
    ScheduleStatus internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle); 

    // MtlInstance mtlInstance() const { return m_mtlInstance; }
    id<MTLDevice> mtlDevice() const { return m_mtlDevice; }
    // MtlPhysicalDevice mtlPhysicalDevice() const { return m_mtlPhysicalDevice; }
    // const MtlPhysicalDeviceProperties& mtlPhysicalDeviceProps() const { return m_mtlPhysicalProps; }
    // MetalDescriptorSetPools& descriptorSetPools() { return *m_descriptorSetPools; }
    // int graphicsFamilyQueueIndex() const { return m_queueFamIndex; }

    // MetalReadbackBufferPool& readbackPool() { return *m_readbackPool; }

    // bool findMemoryType(uint32_t typeFilter, MtlMemoryPropertyFlags properties, uint32_t& outMemType);

    MetalQueues& queues() { return *m_queues; }
    // MetalResources& resources() { return *m_resources; }
    // MetalEventPool& eventPool() { return *m_eventPool; }
    // MetalFencePool& fencePool() { return *m_fencePool; }
    // MetalCounterPool& counterPool() { return *m_counterPool; }
    // MetalMarkerCollector& markerCollector() { return *m_markerCollector; }
    // MetalGc& gc() { return *m_gc; }
    // WorkBundleDb& workDb() { return m_workDb; }

    // Buffer countersBuffer() const { return m_countersBuffer; }

    // BitMask enabledLayers() const { return m_Layers; }
    // BitMask enabledExts() const { return m_Extensions; }
    // BitMask enabledDeviceExts() const { return m_DeviceExtensions; }
    MetalShaderDb& shaderDb() { return *m_shaderDb; }

private:
    // void createSwapchain();
    DeviceInfo m_info;
    DeviceRuntimeInfo m_runtimeInfo;
    MetalShaderDb* m_shaderDb;
    // MtlInstance m_mtlInstance;
    // MtlPhysicalDevice m_mtlPhysicalDevice;
    // MtlPhysicalDeviceMemoryProperties m_mtlMemProps;
    // MtlPhysicalDeviceProperties m_mtlPhysicalProps;
    id<MTLDevice> m_mtlDevice;
    // MetalDescriptorSetPools* m_descriptorSetPools;
    MetalQueues* m_queues;
    MetalResources* m_resources;
    // MetalReadbackBufferPool* m_readbackPool;
    // MetalEventPool* m_eventPool;
    // MetalFencePool* m_fencePool;
    // MetalCounterPool* m_counterPool;
    // MetalGc* m_gc;
    // MetalWorkInformationMap* m_metalWorkInfos;
    // MetalMarkerCollector* m_markerCollector;

    // Buffer m_countersBuffer;

    // int m_queueFamIndex;
    // void testApiFuncs();

    // BitMask m_Layers = {};
    // BitMask m_Extensions = {};
    // BitMask m_DeviceExtensions = {};
};


}

}
