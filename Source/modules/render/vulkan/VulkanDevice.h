#pragma once

#ifndef INCLUDED_T_DEVICE_H
#include <TDevice.h>
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <coalpy.render/Resources.h>
#include <unordered_map>
#include <string>

namespace coalpy
{

class VulkanShaderDb;

namespace render
{

class VulkanDescriptorSetPools;
class VulkanReadbackBufferPool;
class VulkanResources;
class VulkanQueues;
class VulkanEventPool;
class VulkanFencePool;
class VulkanGc;
struct VulkanWorkInformationMap;

class VulkanDevice : public TDevice<VulkanDevice>
{
public:
    VulkanDevice(const DeviceConfig& config);
    virtual ~VulkanDevice();

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
    virtual const DeviceRuntimeInfo& runtimeInfo() const { return m_runtimeInfo; };
    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) override;
    virtual void removeShaderDb() { m_shaderDb = nullptr; }
    virtual IShaderDb* db() override { return (IShaderDb*)m_shaderDb; }
    virtual void beginCollectMarkers(int maxQueryBytes) override;
    virtual MarkerResults endCollectMarkers() override;
    void internalReleaseWorkHandle(WorkHandle handle);
    ScheduleStatus internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle); 

    VkInstance vkInstance() const { return m_vkInstance; }
    VkDevice vkDevice() const { return m_vkDevice; }
    VkPhysicalDevice vkPhysicalDevice() const { return m_vkPhysicalDevice; }

    VulkanDescriptorSetPools& descriptorSetPools() { return *m_descriptorSetPools; }
    int graphicsFamilyQueueIndex() const { return m_queueFamIndex; }

    VulkanReadbackBufferPool& readbackPool() { return *m_readbackPool; }

    bool findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& outMemType);

    VulkanQueues& queues() { return *m_queues; }
    VulkanResources& resources() { return *m_resources; }
    VulkanEventPool& eventPool() { return *m_eventPool; }
    VulkanFencePool& fencePool() { return *m_fencePool; }
    VulkanGc& gc() { return *m_gc; }
    WorkBundleDb& workDb() { return m_workDb; }

private:
    void createSwapchain();
    DeviceInfo m_info;
    DeviceRuntimeInfo m_runtimeInfo;
    VulkanShaderDb* m_shaderDb;
    VkInstance m_vkInstance;
    VkPhysicalDevice m_vkPhysicalDevice;
    VkPhysicalDeviceMemoryProperties m_vkMemProps;
    VkDevice m_vkDevice;
    VulkanDescriptorSetPools* m_descriptorSetPools;
    VulkanQueues* m_queues;
    VulkanResources* m_resources;
    VulkanReadbackBufferPool* m_readbackPool;
    VulkanEventPool* m_eventPool;
    VulkanFencePool* m_fencePool;
    VulkanGc* m_gc;
    VulkanWorkInformationMap* m_vulkanWorkInfos;
    int m_queueFamIndex;

    void testApiFuncs();
};


}

}
