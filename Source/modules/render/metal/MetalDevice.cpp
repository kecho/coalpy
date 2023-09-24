#include <Config.h>
#include <Metal/Metal.h>
#if ENABLE_METAL

#include "MetalDevice.h"
#include "MetalShaderDb.h"
// #if ENABLE_SDL_METAL
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_metal.h>
// #endif
// #include "MetalDescriptorSetPools.h"
// #include "MetalDisplay.h"
#include "MetalResources.h"
// #include "MetalReadbackBufferPool.h"
// #include "MetalWorkBundle.h"
// #include "MetalQueues.h"
// #include "MetalEventPool.h"
// #include "MetalFencePool.h"
// #include "MetalCounterPool.h"
// #include "MetalGc.h"
// #include "MetalUtils.h"
// #include "MetalMarkerCollector.h"
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

// void MetalDevice::testApiFuncs()
// {
// #if TEST_MUTABLE_DESCRIPTORS
//     {
//         std::vector<MtlDescriptorSetLayoutBinding> bindings;
        
//         MtlDescriptorSetLayoutBinding b1 = {};
//         b1.binding = 0;
//         b1.descriptorCount = 1;
//         b1.descriptorType = MTL_DESCRIPTOR_TYPE_MUTABLE_VALVE;
//         b1.stageFlags = MTL_SHADER_STAGE_ALL;
//         bindings.push_back(b1);

//         MtlDescriptorType typeList[] = { MTL_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, MTL_DESCRIPTOR_TYPE_SAMPLED_IMAGE };
//         MtlMutableDescriptorTypeListVALVE valveTypeLists[] =
//         {
//             { 1, typeList }
//         };
        

//         MtlMutableDescriptorTypeCreateInfoVALVE vv = {};
//         vv.sType = MTL_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_VALVE;
//         vv.mutableDescriptorTypeListCount = 1;
//         vv.pMutableDescriptorTypeLists = valveTypeLists;

//         MtlDescriptorSetLayoutCreateInfo setinfo = {};
//         setinfo.sType = MTL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//         setinfo.pNext = &vv;

//         //we are going to have 1 binding
//         setinfo.bindingCount = bindings.size();
//         //no flags
//         setinfo.flags = 0;
//         //point to the camera buffer binding
//         setinfo.pBindings = bindings.data();

//         MtlDescriptorSetLayout outLayout = {};
//         MTL_OK(mtlCreateDescriptorSetLayout(m_mtlDevice, &setinfo, nullptr, &outLayout));
//     }
// #endif

// #if TEST_DESCRIPTORS_LAYOUT_COPY
//     {
//         MtlDescriptorSetLayout layouts[2] = {};
//         {
//             std::vector<MtlDescriptorSetLayoutBinding> bindings;
            
//             MtlDescriptorSetLayoutBinding b0 = {};
//             b0.binding = 0;
//             b0.descriptorCount = 1;
//             b0.descriptorType = MTL_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
//             b0.stageFlags = MTL_SHADER_STAGE_ALL;
//             bindings.push_back(b0);

//             MtlDescriptorSetLayoutBinding b1 = {};
//             b1.binding = 1;
//             b1.descriptorCount = 1;
//             b1.descriptorType = MTL_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
//             b1.stageFlags = MTL_SHADER_STAGE_ALL;
//             bindings.push_back(b1);

//             MtlDescriptorSetLayoutCreateInfo setinfo = {};
//             setinfo.sType = MTL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

//             setinfo.bindingCount = bindings.size();
//             setinfo.pBindings = bindings.data();

//             MTL_OK(mtlCreateDescriptorSetLayout(m_mtlDevice, &setinfo, nullptr, &layouts[0]));
//         }

//         {
//             std::vector<MtlDescriptorSetLayoutBinding> bindings;
            
//             MtlDescriptorSetLayoutBinding b1 = {};
//             b1.binding = 99;
//             b1.descriptorCount = 30;
//             b1.descriptorType = MTL_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
//             b1.stageFlags = MTL_SHADER_STAGE_ALL;
//             bindings.push_back(b1);

//             MtlDescriptorSetLayoutCreateInfo setinfo = {};
//             setinfo.sType = MTL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

//             setinfo.bindingCount = bindings.size();
//             setinfo.pBindings = bindings.data();

//             MTL_OK(mtlCreateDescriptorSetLayout(m_mtlDevice, &setinfo, nullptr, &layouts[1]));
//         }

//         std::vector<MtlDescriptorPoolSize> poolSizes = {
//             { MTL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
//             { MTL_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1 },
//         };
//         MtlDescriptorPoolCreateInfo poolInfo = {};
//         poolInfo.sType = MTL_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//         poolInfo.maxSets = 16;
//         poolInfo.poolSizeCount = (int)poolSizes.size();
//         poolInfo.pPoolSizes = poolSizes.data();

//         MtlDescriptorPool pool = {};
//         MTL_OK(mtlCreateDescriptorPool(m_mtlDevice, &poolInfo, nullptr, &pool));

//         MtlDescriptorSet sets[2];
//         MtlDescriptorSetAllocateInfo allocInfo0 = {};
//         allocInfo0.sType = MTL_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//         allocInfo0.descriptorPool = pool;
//         allocInfo0.descriptorSetCount = 2;
//         allocInfo0.pSetLayouts = layouts;

//         if (mtlAllocateDescriptorSets(m_mtlDevice, &allocInfo0, sets) != MTL_SUCCESS || sets[0] == nullptr)
//         {
//             std::cerr << "ERROR ALLOCATING" << std::endl;
//         }
//         else std::cerr << "ALLOCATING COMPLETE" << std::endl;


//         MtlCopyDescriptorSet copySet = {};
//         copySet.sType = MTL_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
//         copySet.srcSet = sets[0];
//         copySet.srcBinding = 1;
//         copySet.dstSet = sets[1];
//         copySet.dstBinding = 99;
//         copySet.descriptorCount = 1;
//         mtlUpdateDescriptorSets(m_mtlDevice, 0, nullptr, 1, &copySet);

//         mtlDestroyDescriptorPool(m_mtlDevice, pool, nullptr);
//         mtlDestroyDescriptorSetLayout(m_mtlDevice, layouts[0], nullptr);
//         mtlDestroyDescriptorSetLayout(m_mtlDevice, layouts[1], nullptr);
//     }
// #endif
// }

// struct MetalWorkInfo
// {
//     MetalFenceHandle fenceValue = {};
//     MetalDownloadResourceMap downloadMap;
// };

// struct MetalWorkInformationMap
// {
//     std::unordered_map<int, MetalWorkInfo> workMap;
// };

MetalDevice::MetalDevice(const DeviceConfig& config)
:   TDevice<MetalDevice>(config),
    m_shaderDb(nullptr),
    // m_queueFamIndex(-1),
    m_resources(nullptr)
{
    // m_metalWorkInfos = new MetalWorkInformationMap;
    // m_runtimeInfo = { ShaderModel::End };
    // m_mtlMemProps = {};

    // MetalBitMask extensions = 
    //   asFlag(MetalExtensions::SDLExtensions)
    // | asFlag(MetalExtensions::Win32Surface)
    // | asFlag(MetalExtensions::KHRSurface)
    // | asFlag(MetalExtensions::CustomBorderColorSampler)
    // | asFlag(MetalExtensions::MinMaxFilterSampler);

    // if (((int)config.flags & (int)DeviceFlags::EnableDebug) != 0) 
    //     extensions |= asFlag(MetalExtensions::DebugReport);

    // MetalBitMask layers =
    //   asFlag(MetalLayers::NVOptimus)
    // | asFlag(MetalLayers::KHRONOSvalidation) 
    // | asFlag(MetalLayers::LUNARGStandardValidation);
    
    // createMetalInstance(layers, extensions, m_mtlInstance, m_Layers, m_Extensions);

    // std::vector<DeviceInfo> deviceInfos;
    // std::vector<MtlPhysicalDevice> physicalDevices;
    // enumerateMetalDevices(deviceInfos, physicalDevices);
    // int selectedDeviceIdx = std::min<int>(std::max<int>(config.index, 0), (int)physicalDevices.size() - 1);
    // m_info = deviceInfos[selectedDeviceIdx];
    // m_mtlPhysicalDevice = physicalDevices[selectedDeviceIdx];

    // m_queueFamIndex = getGraphicsComputeQueueFamilyIndex(m_mtlPhysicalDevice);
    // CPY_ASSERT(m_queueFamIndex != -1);

    // MetalBitMask deviceExts = 
    //   asFlag(MetalDeviceExtensions::KHRSwapChain)
    // | asFlag(MetalDeviceExtensions::KHRMaintenance)
    // | asFlag(MetalDeviceExtensions::GoogleHLSLFunctionality1)
    // | asFlag(MetalDeviceExtensions::GooleUserType);
    
    // createMetalDevice(
    //     m_mtlPhysicalDevice,
    //     deviceExts,
    //     m_queueFamIndex,
    //     m_mtlDevice,
    //     m_DeviceExtensions);

    // if (m_queueFamIndex == -1)
    //     std::cerr << "Could not find a compute queue for device selected" << std::endl;

    // if (config.shaderDb)
    // {
    //     m_shaderDb = static_cast<MetalShaderDb*>(config.shaderDb);
    //     CPY_ASSERT_MSG(m_shaderDb->parentDevice() == nullptr, "shader database can only belong to 1 and only 1 device");
        
    //     m_shaderDb->setParentDevice(this, &m_runtimeInfo);
    // }

    // mtlGetPhysicalDeviceMemoryProperties(m_mtlPhysicalDevice, &m_mtlMemProps);
    // mtlGetPhysicalDeviceProperties(m_mtlPhysicalDevice, &m_mtlPhysicalProps);

    // m_fencePool = new MetalFencePool(*this);
    // m_eventPool = new MetalEventPool(*this);
    // m_queues =  new MetalQueues(*this, *m_fencePool, *m_eventPool);
    // m_gc = new MetalGc(125, *this);
    m_resources = new MetalResources(*this, m_workDb);
    // m_descriptorSetPools = new MetalDescriptorSetPools(*this);
    // m_readbackPool = new MetalReadbackBufferPool(*this);
    // m_counterPool = new MetalCounterPool(*this);
    // m_markerCollector = new MetalMarkerCollector(*this);

    // m_gc->start();
    
    // //register counter as a buffer handle
    // {
    //     BufferDesc bufferDesc;
    //     bufferDesc.format = Format::RGBA_32_UINT;
    //     bufferDesc.elementCount = 1;
    //     bufferDesc.memFlags = (MemFlags)0u;
    //     m_countersBuffer = m_resources->createBuffer(bufferDesc, m_counterPool->resource(), ResourceSpecialFlag_None);
    // }

    // testApiFuncs();
}

MetalDevice::~MetalDevice()
{
    // //sync device here, so deletion is clean.
    // if (m_queues)
    //     for (int workType = 0; workType < (int)WorkType::Count; ++workType)
    //         m_queues->waitForAllWorkOnCpu((WorkType)workType);

    // if (m_shaderDb && m_shaderDb->parentDevice() == this)
    // {
    //     m_shaderDb->purgePayloads();
    //     m_shaderDb->setParentDevice(nullptr, nullptr);
    // }

    // for (auto p : m_metalWorkInfos->workMap)
    //     for (auto dl : p.second.downloadMap)
    //         readbackPool().free(dl.second.memoryBlock);

    // m_queues->releaseResources();

    // m_resources->release(m_countersBuffer);

    // delete m_markerCollector;
    // m_markerCollector = nullptr;
    // delete m_readbackPool;
    // m_readbackPool = nullptr;
    delete m_resources;
    m_resources = nullptr;
    // delete m_descriptorSetPools;
    // m_descriptorSetPools = nullptr;
    // delete m_gc;
    // m_gc = nullptr;
    // delete m_counterPool;
    // m_counterPool = nullptr;
    // delete m_queues;
    // m_queues = nullptr;
    // delete m_eventPool;
    // m_eventPool = nullptr;
    // delete m_fencePool;
    // m_fencePool = nullptr;

    // mtlDestroyDevice(m_mtlDevice, nullptr); 
    // destroyMetalInstance(m_mtlInstance);    
    // delete m_metalWorkInfos;
}

void MetalDevice::enumerate(std::vector<DeviceInfo>& outputList)
{
    auto devices = MTLCopyAllDevices();
    outputList.reserve(devices.count);
    int index = 0;
    for(id<MTLDevice> device in devices) {
        DeviceInfo deviceInfo;
        deviceInfo.index = index++;
        deviceInfo.valid = true;
        deviceInfo.name = device.name.UTF8String;

        outputList.push_back(deviceInfo);
    }
    [devices release];
    // std::vector<MtlPhysicalDevice> unused;
    // enumerateMetalDevices(outputList, unused);
}


// bool MetalDevice::findMemoryType(uint32_t typeFilter, MtlMemoryPropertyFlags properties, uint32_t& memType)
// {
//     for (uint32_t i = 0; i < m_mtlMemProps.memoryTypeCount; i++)
//     {
//         if ((typeFilter & (1 << i)) && (m_mtlMemProps.memoryTypes[i].propertyFlags & properties) == properties)
//         {
//             memType = i;
//             return true;
//         }
//     }

//     return false;
// }

void MetalDevice::beginCollectMarkers(int maxQueryBytes)
{
    // m_markerCollector->beginCollection(maxQueryBytes);
}

MarkerResults MetalDevice::endCollectMarkers()
{
        MarkerResults results = {};
        return results;
        // TODO (Apoorva)
//     return m_markerCollector->endCollection();
}

TextureResult MetalDevice::createTexture(const TextureDesc& desc)
{
    return TextureResult { ResourceResult::Ok, { -1 } };
    // TODO (Apoorva)
    // return m_resources->createTexture(desc);
}

TextureResult MetalDevice::recreateTexture(Texture texture, const TextureDesc& desc)
{
    return TextureResult { ResourceResult::Ok, { -1 } };
    // TODO (Apoorva)
    // return m_resources->recreateTexture(texture, desc);
}

BufferResult  MetalDevice::createBuffer (const BufferDesc& config)
{
    return m_resources->createBuffer(config);
    // TODO (Apoorva)
    // return m_resources->createBuffer(config, MTL_NULL_HANDLE);
}

SamplerResult MetalDevice::createSampler (const SamplerDesc& config)
{
    return SamplerResult { ResourceResult::Ok, { -1 } };
    // TODO (Apoorva)
    // return m_resources->createSampler(config);
}

InResourceTableResult MetalDevice::createInResourceTable  (const ResourceTableDesc& config)
{
    return InResourceTableResult { ResourceResult::Ok, { -1 } };
    // TODO (Apoorva)
    // return m_resources->createInResourceTable(config);
}

OutResourceTableResult MetalDevice::createOutResourceTable (const ResourceTableDesc& config)
{
    return OutResourceTableResult { ResourceResult::Ok, { -1 } };
    // TODO (Apoorva)
    // return m_resources->createOutResourceTable(config);
}

SamplerTableResult  MetalDevice::createSamplerTable (const ResourceTableDesc& config)
{
    return SamplerTableResult { ResourceResult::Ok, { -1 } };
    // TODO (Apoorva)
    // return m_resources->createSamplerTable(config);
}

void MetalDevice::getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo)
{
    // m_resources->getResourceMemoryInfo(handle, memInfo);
}

WaitStatus MetalDevice::waitOnCpu(WorkHandle handle, int milliseconds)
{
    return WaitStatus { WaitErrorType::NotReady, "" };
    // TODO (Apoorva)
//     auto workInfoIt = m_metalWorkInfos->workMap.find(handle.handleId);
//     if (workInfoIt == m_metalWorkInfos->workMap.end())
//         return WaitStatus { WaitErrorType::Invalid, "Invalid work handle." };    

//     if (milliseconds != 0u)
//         m_fencePool->waitOnCpu(workInfoIt->second.fenceValue, milliseconds < 0 ? ~0ull : (uint64_t)milliseconds);

//     m_fencePool->updateState(workInfoIt->second.fenceValue);
//     if (m_fencePool->isSignaled(workInfoIt->second.fenceValue))
//         return WaitStatus { WaitErrorType::Ok, "" };
//     else
//         return WaitStatus { WaitErrorType::NotReady, "" };
}

DownloadStatus MetalDevice::getDownloadStatus(WorkHandle bundle, ResourceHandle handle, int mipLevel, int arraySlice)
{
//     auto it = m_metalWorkInfos->workMap.find(bundle.handleId);
//     if (it == m_metalWorkInfos->workMap.end())
//         return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

//     ResourceDownloadKey downloadKey { handle, mipLevel, arraySlice };
//     auto downloadStateIt = it->second.downloadMap.find(downloadKey);
//     if (downloadStateIt == it->second.downloadMap.end())
//         return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

//     auto& downloadState = downloadStateIt->second;
//     m_fencePool->updateState(downloadState.fenceValue);
//     if (m_fencePool->isSignaled(downloadState.fenceValue))
//     {
//         return DownloadStatus {
//             DownloadResult::Ok,
//             downloadState.memoryBlock.mappedMemory,
//             downloadState.requestedSize,
//             downloadState.rowPitch,
//             downloadState.width,
//             downloadState.height,
//             downloadState.depth,
//         };
//     }
    return DownloadStatus { DownloadResult::NotReady, nullptr, 0u };
}

void MetalDevice::release(ResourceHandle resource)
{
    // m_resources->release(resource);
}

void MetalDevice::release(ResourceTable table)
{
//     m_resources->release(table);
}

SmartPtr<IDisplay> MetalDevice::createDisplay (const DisplayConfig& config)
{
    return nullptr;
    // TODO (Apoorva)
    // return new MetalDisplay(config, *this);
}

void MetalDevice::internalReleaseWorkHandle(WorkHandle handle)
{
//     auto workInfoIt = m_metalWorkInfos->workMap.find(handle.handleId);
//     CPY_ASSERT(workInfoIt != m_metalWorkInfos->workMap.end());
//     if (workInfoIt == m_metalWorkInfos->workMap.end())
//         return;

//     for (auto downloadStatePair : workInfoIt->second.downloadMap)
//     {
//         m_fencePool->free(downloadStatePair.second.fenceValue);
//         readbackPool().free(downloadStatePair.second.memoryBlock);
//     }

//     m_fencePool->free(workInfoIt->second.fenceValue);
//     m_metalWorkInfos->workMap.erase(workInfoIt);
}

ScheduleStatus MetalDevice::internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle)
{
    ScheduleStatus status;
//     status.workHandle = workHandle;
    
//     MetalWorkBundle metalWorkBundle(*this);
//     {
//         m_workDb.lock();
//         WorkBundle& workBundle = m_workDb.unsafeGetWorkBundle(workHandle);
//         metalWorkBundle.load(workBundle);
//         m_workDb.unlock();
//     }

//     MetalFenceHandle fenceValue = metalWorkBundle.execute(commandLists, listCounts);

//     {
//         m_workDb.lock();
//         MetalWorkInfo workInfo;
//         workInfo.fenceValue = fenceValue;
//         metalWorkBundle.getDownloadResourceMap(workInfo.downloadMap);
//         m_metalWorkInfos->workMap[workHandle.handleId] = std::move(workInfo);
//         m_workDb.unlock();
//     }

    return status;
}

}
}

#endif // ENABLE_METAL