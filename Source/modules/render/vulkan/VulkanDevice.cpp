#include <Config.h>
#if ENABLE_VULKAN

#include "VulkanDevice.h"
#include "VulkanShaderDb.h"
#if ENABLE_SDL_VULKAN
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif
#include "VulkanDescriptorSetPools.h"
#include "VulkanDisplay.h"
#include "VulkanResources.h"
#include "VulkanReadbackBufferPool.h"
#include "VulkanWorkBundle.h"
#include "VulkanQueues.h"
#include "VulkanEventPool.h"
#include "VulkanFencePool.h"
#include "VulkanCounterPool.h"
#include "VulkanGc.h"
#include "VulkanUtils.h"
#include "VulkanMarkerCollector.h"
#include <coalpy.render/ShaderDefs.h>
#include <iostream>
#include <set>
#include <vector>

#define TEST_MUTABLE_DESCRIPTORS 0
#define TEST_DESCRIPTORS_LAYOUT_COPY 0

namespace coalpy
{
namespace render
{

void VulkanDevice::testApiFuncs()
{
#if TEST_MUTABLE_DESCRIPTORS
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        
        VkDescriptorSetLayoutBinding b1 = {};
        b1.binding = 0;
        b1.descriptorCount = 1;
        b1.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_VALVE;
        b1.stageFlags = VK_SHADER_STAGE_ALL;
        bindings.push_back(b1);

        VkDescriptorType typeList[] = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE };
        VkMutableDescriptorTypeListVALVE valveTypeLists[] =
        {
            { 1, typeList }
        };
        

        VkMutableDescriptorTypeCreateInfoVALVE vv = {};
        vv.sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_VALVE;
        vv.mutableDescriptorTypeListCount = 1;
        vv.pMutableDescriptorTypeLists = valveTypeLists;

        VkDescriptorSetLayoutCreateInfo setinfo = {};
        setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setinfo.pNext = &vv;

        //we are going to have 1 binding
        setinfo.bindingCount = bindings.size();
        //no flags
        setinfo.flags = 0;
        //point to the camera buffer binding
        setinfo.pBindings = bindings.data();

        VkDescriptorSetLayout outLayout = {};
        VK_OK(vkCreateDescriptorSetLayout(m_vkDevice, &setinfo, nullptr, &outLayout));
    }
#endif

#if TEST_DESCRIPTORS_LAYOUT_COPY
    {
        VkDescriptorSetLayout layouts[2] = {};
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            
            VkDescriptorSetLayoutBinding b0 = {};
            b0.binding = 0;
            b0.descriptorCount = 1;
            b0.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            b0.stageFlags = VK_SHADER_STAGE_ALL;
            bindings.push_back(b0);

            VkDescriptorSetLayoutBinding b1 = {};
            b1.binding = 1;
            b1.descriptorCount = 1;
            b1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            b1.stageFlags = VK_SHADER_STAGE_ALL;
            bindings.push_back(b1);

            VkDescriptorSetLayoutCreateInfo setinfo = {};
            setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

            setinfo.bindingCount = bindings.size();
            setinfo.pBindings = bindings.data();

            VK_OK(vkCreateDescriptorSetLayout(m_vkDevice, &setinfo, nullptr, &layouts[0]));
        }

        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            
            VkDescriptorSetLayoutBinding b1 = {};
            b1.binding = 99;
            b1.descriptorCount = 30;
            b1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            b1.stageFlags = VK_SHADER_STAGE_ALL;
            bindings.push_back(b1);

            VkDescriptorSetLayoutCreateInfo setinfo = {};
            setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

            setinfo.bindingCount = bindings.size();
            setinfo.pBindings = bindings.data();

            VK_OK(vkCreateDescriptorSetLayout(m_vkDevice, &setinfo, nullptr, &layouts[1]));
        }

        std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1 },
        };
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = 16;
        poolInfo.poolSizeCount = (int)poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();

        VkDescriptorPool pool = {};
        VK_OK(vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &pool));

        VkDescriptorSet sets[2];
        VkDescriptorSetAllocateInfo allocInfo0 = {};
        allocInfo0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo0.descriptorPool = pool;
        allocInfo0.descriptorSetCount = 2;
        allocInfo0.pSetLayouts = layouts;

        if (vkAllocateDescriptorSets(m_vkDevice, &allocInfo0, sets) != VK_SUCCESS || sets[0] == nullptr)
        {
            std::cerr << "ERROR ALLOCATING" << std::endl;
        }
        else std::cerr << "ALLOCATING COMPLETE" << std::endl;


        VkCopyDescriptorSet copySet = {};
        copySet.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
        copySet.srcSet = sets[0];
        copySet.srcBinding = 1;
        copySet.dstSet = sets[1];
        copySet.dstBinding = 99;
        copySet.descriptorCount = 1;
        vkUpdateDescriptorSets(m_vkDevice, 0, nullptr, 1, &copySet);

        vkDestroyDescriptorPool(m_vkDevice, pool, nullptr);
        vkDestroyDescriptorSetLayout(m_vkDevice, layouts[0], nullptr);
        vkDestroyDescriptorSetLayout(m_vkDevice, layouts[1], nullptr);
    }
#endif
}

struct VulkanWorkInfo
{
    VulkanFenceHandle fenceValue = {};
    VulkanDownloadResourceMap downloadMap;
};

struct VulkanWorkInformationMap
{
    std::unordered_map<int, VulkanWorkInfo> workMap;
};

VulkanDevice::VulkanDevice(const DeviceConfig& config)
:   TDevice<VulkanDevice>(config),
    m_shaderDb(nullptr),
    m_queueFamIndex(-1),
    m_resources(nullptr)
{
    m_vulkanWorkInfos = new VulkanWorkInformationMap;
    m_runtimeInfo = { ShaderModel::End };
    m_vkMemProps = {};

    VulkanBitMask extensions = 
      asFlag(VulkanExtensions::SDLExtensions)
    | asFlag(VulkanExtensions::Win32Surface)
    | asFlag(VulkanExtensions::KHRSurface)
    | asFlag(VulkanExtensions::CustomBorderColorSampler)
    | asFlag(VulkanExtensions::MinMaxFilterSampler);

    if (((int)config.flags & (int)DeviceFlags::EnableDebug) != 0) 
        extensions |= asFlag(VulkanExtensions::DebugReport);

    VulkanBitMask layers =
      asFlag(VulkanLayers::NVOptimus)
    | asFlag(VulkanLayers::KHRONOSvalidation) 
    | asFlag(VulkanLayers::LUNARGStandardValidation);
    
    createVulkanInstance(layers, extensions, m_vkInstance, m_Layers, m_Extensions);

    std::vector<DeviceInfo> deviceInfos;
    std::vector<VkPhysicalDevice> physicalDevices;
    enumerateVulkanDevices(deviceInfos, physicalDevices);
    int selectedDeviceIdx = std::min<int>(std::max<int>(config.index, 0), (int)physicalDevices.size() - 1);
    m_info = deviceInfos[selectedDeviceIdx];
    m_vkPhysicalDevice = physicalDevices[selectedDeviceIdx];

    m_queueFamIndex = getGraphicsComputeQueueFamilyIndex(m_vkPhysicalDevice);
    CPY_ASSERT(m_queueFamIndex != -1);

    VulkanBitMask deviceExts = 
      asFlag(VulkanDeviceExtensions::KHRSwapChain)
    | asFlag(VulkanDeviceExtensions::KHRMaintenance)
    | asFlag(VulkanDeviceExtensions::GoogleHLSLFunctionality1)
    | asFlag(VulkanDeviceExtensions::GooleUserType);
    
    createVulkanDevice(
        m_vkPhysicalDevice,
        deviceExts,
        m_queueFamIndex,
        m_vkDevice,
        m_DeviceExtensions);

    if (m_queueFamIndex == -1)
        std::cerr << "Could not find a compute queue for device selected" << std::endl;

    if (config.shaderDb)
    {
        m_shaderDb = static_cast<VulkanShaderDb*>(config.shaderDb);
        CPY_ASSERT_MSG(m_shaderDb->parentDevice() == nullptr, "shader database can only belong to 1 and only 1 device");
        
        m_shaderDb->setParentDevice(this, &m_runtimeInfo);
    }

    vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkMemProps);
    vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_vkPhysicalProps);

    m_fencePool = new VulkanFencePool(*this);
    m_eventPool = new VulkanEventPool(*this);
    m_queues =  new VulkanQueues(*this, *m_fencePool, *m_eventPool);
    m_gc = new VulkanGc(125, *this);
    m_resources = new VulkanResources(*this, m_workDb);
    m_descriptorSetPools = new VulkanDescriptorSetPools(*this);
    m_readbackPool = new VulkanReadbackBufferPool(*this);
    m_counterPool = new VulkanCounterPool(*this);
    m_markerCollector = new VulkanMarkerCollector(*this);

    m_gc->start();
    
    //register counter as a buffer handle
    {
        BufferDesc bufferDesc;
        bufferDesc.format = Format::RGBA_32_UINT;
        bufferDesc.elementCount = 1;
        bufferDesc.memFlags = (MemFlags)0u;
        m_countersBuffer = m_resources->createBuffer(bufferDesc, m_counterPool->resource(), ResourceSpecialFlag_None);
    }

    testApiFuncs();
}

VulkanDevice::~VulkanDevice()
{
    //sync device here, so deletion is clean.
    if (m_queues)
        for (int workType = 0; workType < (int)WorkType::Count; ++workType)
            m_queues->waitForAllWorkOnCpu((WorkType)workType);

    if (m_shaderDb && m_shaderDb->parentDevice() == this)
    {
        m_shaderDb->purgePayloads();
        m_shaderDb->setParentDevice(nullptr, nullptr);
    }

    for (auto p : m_vulkanWorkInfos->workMap)
        for (auto dl : p.second.downloadMap)
            readbackPool().free(dl.second.memoryBlock);

    m_queues->releaseResources();

    m_resources->release(m_countersBuffer);

    delete m_markerCollector;
    m_markerCollector = nullptr;
    delete m_readbackPool;
    m_readbackPool = nullptr;
    delete m_resources;
    m_resources = nullptr;
    delete m_descriptorSetPools;
    m_descriptorSetPools = nullptr;
    delete m_gc;
    m_gc = nullptr;
    delete m_counterPool;
    m_counterPool = nullptr;
    delete m_queues;
    m_queues = nullptr;
    delete m_eventPool;
    m_eventPool = nullptr;
    delete m_fencePool;
    m_fencePool = nullptr;

    vkDestroyDevice(m_vkDevice, nullptr); 
    destroyVulkanInstance(m_vkInstance);    
    delete m_vulkanWorkInfos;
}

void VulkanDevice::enumerate(std::vector<DeviceInfo>& outputList)
{
    std::vector<VkPhysicalDevice> unused;
    enumerateVulkanDevices(outputList, unused);
}


bool VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& memType)
{
    for (uint32_t i = 0; i < m_vkMemProps.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (m_vkMemProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            memType = i;
            return true;
        }
    }

    return false;
}

void VulkanDevice::beginCollectMarkers(int maxQueryBytes)
{
    m_markerCollector->beginCollection(maxQueryBytes);
}

MarkerResults VulkanDevice::endCollectMarkers()
{
    return m_markerCollector->endCollection();
}

TextureResult VulkanDevice::createTexture(const TextureDesc& desc)
{
    return m_resources->createTexture(desc);
}

TextureResult VulkanDevice::recreateTexture(Texture texture, const TextureDesc& desc)
{
    return m_resources->recreateTexture(texture, desc);
}

BufferResult  VulkanDevice::createBuffer (const BufferDesc& config)
{
    return m_resources->createBuffer(config, VK_NULL_HANDLE);
}

SamplerResult VulkanDevice::createSampler (const SamplerDesc& config)
{
    return m_resources->createSampler(config);
}

InResourceTableResult VulkanDevice::createInResourceTable  (const ResourceTableDesc& config)
{
    return m_resources->createInResourceTable(config);
}

OutResourceTableResult VulkanDevice::createOutResourceTable (const ResourceTableDesc& config)
{
    return m_resources->createOutResourceTable(config);
}

SamplerTableResult  VulkanDevice::createSamplerTable (const ResourceTableDesc& config)
{
    return m_resources->createSamplerTable(config);
}

void VulkanDevice::getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo)
{
    m_resources->getResourceMemoryInfo(handle, memInfo);
}

WaitStatus VulkanDevice::waitOnCpu(WorkHandle handle, int milliseconds)
{
    auto workInfoIt = m_vulkanWorkInfos->workMap.find(handle.handleId);
    if (workInfoIt == m_vulkanWorkInfos->workMap.end())
        return WaitStatus { WaitErrorType::Invalid, "Invalid work handle." };    

    if (milliseconds != 0u)
        m_fencePool->waitOnCpu(workInfoIt->second.fenceValue, milliseconds < 0 ? ~0ull : (uint64_t)milliseconds);

    m_fencePool->updateState(workInfoIt->second.fenceValue);
    if (m_fencePool->isSignaled(workInfoIt->second.fenceValue))
        return WaitStatus { WaitErrorType::Ok, "" };
    else
        return WaitStatus { WaitErrorType::NotReady, "" };
}

DownloadStatus VulkanDevice::getDownloadStatus(WorkHandle bundle, ResourceHandle handle, int mipLevel, int arraySlice)
{
    auto it = m_vulkanWorkInfos->workMap.find(bundle.handleId);
    if (it == m_vulkanWorkInfos->workMap.end())
        return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

    ResourceDownloadKey downloadKey { handle, mipLevel, arraySlice };
    auto downloadStateIt = it->second.downloadMap.find(downloadKey);
    if (downloadStateIt == it->second.downloadMap.end())
        return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

    auto& downloadState = downloadStateIt->second;
    m_fencePool->updateState(downloadState.fenceValue);
    if (m_fencePool->isSignaled(downloadState.fenceValue))
    {
        return DownloadStatus {
            DownloadResult::Ok,
            downloadState.memoryBlock.mappedMemory,
            downloadState.requestedSize,
            downloadState.rowPitch,
            downloadState.width,
            downloadState.height,
            downloadState.depth,
        };
    }
    return DownloadStatus { DownloadResult::NotReady, nullptr, 0u };
}

void VulkanDevice::release(ResourceHandle resource)
{
    m_resources->release(resource);
}

void VulkanDevice::release(ResourceTable table)
{
    m_resources->release(table);
}

SmartPtr<IDisplay> VulkanDevice::createDisplay(const DisplayConfig& config)
{
    return new VulkanDisplay(config, *this);
}

void VulkanDevice::internalReleaseWorkHandle(WorkHandle handle)
{
    auto workInfoIt = m_vulkanWorkInfos->workMap.find(handle.handleId);
    CPY_ASSERT(workInfoIt != m_vulkanWorkInfos->workMap.end());
    if (workInfoIt == m_vulkanWorkInfos->workMap.end())
        return;

    for (auto downloadStatePair : workInfoIt->second.downloadMap)
    {
        m_fencePool->free(downloadStatePair.second.fenceValue);
        readbackPool().free(downloadStatePair.second.memoryBlock);
    }

    m_fencePool->free(workInfoIt->second.fenceValue);
    m_vulkanWorkInfos->workMap.erase(workInfoIt);
}

ScheduleStatus VulkanDevice::internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle)
{
    ScheduleStatus status;
    status.workHandle = workHandle;
    
    VulkanWorkBundle vulkanWorkBundle(*this);
    {
        m_workDb.lock();
        WorkBundle& workBundle = m_workDb.unsafeGetWorkBundle(workHandle);
        vulkanWorkBundle.load(workBundle);
        m_workDb.unlock();
    }

    VulkanFenceHandle fenceValue = vulkanWorkBundle.execute(commandLists, listCounts);

    {
        m_workDb.lock();
        VulkanWorkInfo workInfo;
        workInfo.fenceValue = fenceValue;
        vulkanWorkBundle.getDownloadResourceMap(workInfo.downloadMap);
        m_vulkanWorkInfos->workMap[workHandle.handleId] = std::move(workInfo);
        m_workDb.unlock();
    }

    return status;
}

}
}

#endif // ENABLE_VULKAN