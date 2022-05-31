#include <Config.h>

#if ENABLE_DX12

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

#if _DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid")
#endif

#include <dxgi1_6.h>
#include <d3d12.h>
#include <algorithm>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <iostream>
#include <vector>

#include "Dx12Device.h"
#include "Dx12Queues.h"
#include "Dx12Display.h"
#include "Dx12ShaderDb.h"
#include "Dx12ResourceCollection.h"
#include "Dx12DescriptorPool.h"
#include "Dx12WorkBundle.h"
#include "Dx12BufferPool.h"
#include "Dx12CounterPool.h"
#include "Dx12Fence.h"
#include "Dx12PixApi.h"
#include "Dx12Gc.h"

namespace coalpy
{
namespace render
{

namespace 
{

enum : int
{
    Tier3MaxNumTables = 8
};

struct CardInfos
{
    bool initialized = false;
    int  refs = 0;
    IDXGIFactory4* dxgiFactory = nullptr;
    std::vector<IDXGIAdapter4*> cards;

    void free()
    {
        refs = 0;
        initialized = false;
        if (dxgiFactory)
        {
            dxgiFactory->Release();
            dxgiFactory = nullptr;
        }

        for (auto* c : cards)
            if (c)
                c->Release();

        cards.clear();
    }
} g_cardInfos;

void cacheCardInfos(CardInfos& cardInfos)
{
    ++cardInfos.refs;

    if (cardInfos.initialized)
        return;

    int factoryFlags = 0;
#if _DEBUG
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    DX_OK(CreateDXGIFactory2(factoryFlags, DX_RET(cardInfos.dxgiFactory)));
    cardInfos.initialized = true;

    if (!cardInfos.dxgiFactory)
        return;

    bool finish = false;
    while (!finish)
    {
        SmartPtr<IDXGIAdapter1> adapter;
        auto result = cardInfos.dxgiFactory->EnumAdapters1((UINT)cardInfos.cards.size(), (IDXGIAdapter1**)&adapter);
        if (result == DXGI_ERROR_NOT_FOUND)
        {
            finish = true;
            continue;
        }

        SmartPtr<IDXGIAdapter4> adapter4;
        DX_OK(adapter->QueryInterface((IDXGIAdapter4**)&adapter4));
        
        adapter4->AddRef();
        cardInfos.cards.push_back(adapter4);
    }
}

void shutdownCardInfos(CardInfos& cardInfos)
{
    --cardInfos.refs;
    if (cardInfos.refs == 0)
        cardInfos.free();
}

ID3D12RootSignature* createComputeRootSignature(ID3D12Device2& device, const D3D12_FEATURE_DATA_D3D12_OPTIONS& featureOpts, unsigned maxResourceTables)
{
    unsigned maxDescriptors[(int)Dx12Device::TableTypes::Count] = {};
    switch (featureOpts.ResourceBindingTier)
    {
    case D3D12_RESOURCE_BINDING_TIER_1:
        maxDescriptors[(int)Dx12Device::TableTypes::Srv] = 128;
        maxDescriptors[(int)Dx12Device::TableTypes::Uav] = 8;
        maxDescriptors[(int)Dx12Device::TableTypes::Cbv] = 14;
        maxDescriptors[(int)Dx12Device::TableTypes::Sampler] = 16;
        break;
    case D3D12_RESOURCE_BINDING_TIER_2:
        maxDescriptors[(int)Dx12Device::TableTypes::Srv] = UINT_MAX;
        maxDescriptors[(int)Dx12Device::TableTypes::Uav] = 64;
        maxDescriptors[(int)Dx12Device::TableTypes::Cbv] = 14;
        maxDescriptors[(int)Dx12Device::TableTypes::Sampler] = 2048;
        break;
    case D3D12_RESOURCE_BINDING_TIER_3:
    default:
        maxDescriptors[(int)Dx12Device::TableTypes::Srv] = UINT_MAX;
        maxDescriptors[(int)Dx12Device::TableTypes::Uav] = UINT_MAX;
        maxDescriptors[(int)Dx12Device::TableTypes::Cbv] = UINT_MAX;
        maxDescriptors[(int)Dx12Device::TableTypes::Sampler] = UINT_MAX;
    }

    unsigned totalNumberTables = maxResourceTables * (unsigned)Dx12Device::TableTypes::Count;
    D3D12_ROOT_PARAMETER1 rootParams[(int)Tier3MaxNumTables]; //allocate max
    D3D12_DESCRIPTOR_RANGE1 ranges[(int)Tier3MaxNumTables];//allocate max

    static const D3D12_DESCRIPTOR_RANGE_TYPE g_rangeTypes[(int)Dx12Device::TableTypes::Count] = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
    };

    for (int tableTypeId = 0; tableTypeId < (int)Dx12Device::TableTypes::Count; ++tableTypeId)
    {
        for (int tableId = 0; tableId < (int)maxResourceTables; ++tableId)
        {
            auto tableType = (Dx12Device::TableTypes)tableTypeId;
            int tableIndex = tableTypeId * (int)maxResourceTables + tableId;
            auto& rootParam = rootParams[tableIndex];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            {
                auto& typeRange = ranges[tableIndex];
                typeRange.RangeType = g_rangeTypes[tableTypeId];
                typeRange.NumDescriptors = maxDescriptors[tableTypeId]; 
                typeRange.BaseShaderRegister = 0;
                typeRange.RegisterSpace = tableId;
                typeRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
                typeRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                rootParam.DescriptorTable.pDescriptorRanges = &typeRange;
            }

            rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1 = {};
    desc.Desc_1_1.NumParameters = totalNumberTables;
    desc.Desc_1_1.pParameters = rootParams;
    desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3D12RootSignature* outRootSignature = nullptr;
    SmartPtr<ID3DBlob> rootSignatureBlob;
    DX_OK(D3D12SerializeVersionedRootSignature(&desc, (ID3DBlob**)&rootSignatureBlob, nullptr));
    DX_OK(device.CreateRootSignature(0u, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), DX_RET(outRootSignature)));
    return outRootSignature;
}

ID3D12CommandSignature* createIndirectDispatchCommandSignature(ID3D12Device2& device)
{
    ID3D12CommandSignature* indirectDispatchCmdSignature = nullptr;
    
    D3D12_COMMAND_SIGNATURE_DESC desc = {};
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
    desc.NumArgumentDescs = 1;
    desc.pArgumentDescs = &argDesc;

    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

    DX_OK(device.CreateCommandSignature(&desc, nullptr, DX_RET(indirectDispatchCmdSignature)));

    return indirectDispatchCmdSignature;
}

}

struct Dx12WorkInfo
{
    UINT64 fenceValue = {};
    Dx12DownloadResourceMap downloadMap;
};

struct Dx12WorkInformationMap
{
    std::unordered_map<int, Dx12WorkInfo> workMap;
};

Dx12Device::Dx12Device(const DeviceConfig& config)
: TDevice<Dx12Device>(config)
, m_debugLayer(nullptr)
, m_device(nullptr)
, m_shaderDb(nullptr)
, m_counterPool(nullptr)
, m_readbackPool(nullptr)
, m_gc(nullptr)
, m_maxResourceTables(0u)
, m_supportedSM(ShaderModel::End)
{
    m_info = {};
    m_dx12WorkInfos = new Dx12WorkInformationMap;
    cacheCardInfos(g_cardInfos);

    if (((int)config.flags & (int)DeviceFlags::EnableDebug) != 0)
    {
        DX_OK(D3D12GetDebugInterface(DX_RET(m_debugLayer)));
        if (m_debugLayer)
            m_debugLayer->EnableDebugLayer();
    }

    if (g_cardInfos.cards.empty())
        return;

    int selectedDeviceIdx = std::min<int>(std::max<int>(config.index, 0), (int)g_cardInfos.cards.size() - 1);
    IDXGIAdapter4* selectedCard = g_cardInfos.cards[selectedDeviceIdx];
    if (selectedCard == nullptr)
        return;

    DXGI_ADAPTER_DESC1 desc;
    DX_OK(selectedCard->GetDesc1(&desc));
    std::wstring wdesc = desc.Description;

    DX_OK(D3D12CreateDevice(selectedCard, D3D_FEATURE_LEVEL_11_0, DX_RET(m_device)));

    if (!m_device)
        return;

    m_info.index = selectedDeviceIdx;
    m_info.valid = true;
    m_info.name = ws2s(wdesc);

    m_descriptors = new Dx12DescriptorPool(*this);
    m_queues = new Dx12Queues(*this);


    m_counterPool = new Dx12CounterPool(*this);

    m_gc = new Dx12Gc(125/*every 125ms tick*/, *m_counterPool, m_queues->getFence(WorkType::Graphics));

    m_resources = new Dx12ResourceCollection(*this, m_workDb);

    m_readbackPool = new Dx12BufferPool(*this, true);

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpts = {};
    DX_OK(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpts, sizeof(featureOpts)));

    m_maxResourceTables = featureOpts.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 ? Tier3MaxNumTables : 1u;

    m_computeRootSignature = createComputeRootSignature(*m_device, featureOpts, m_maxResourceTables);

    m_indirectDispatchCommandSignature = createIndirectDispatchCommandSignature(*m_device);


    D3D12_FEATURE_DATA_SHADER_MODEL highestSM = { D3D_SHADER_MODEL_6_5 };
    DX_OK(m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &highestSM, sizeof(highestSM)));

    if (highestSM.HighestShaderModel < D3D_SHADER_MODEL_6_0)
    {
        std::cerr << "Shader model supported by this graphics card is too low. Attempting SM 6_3";
        m_supportedSM = ShaderModel::Sm6_0;
    }
    else if (highestSM.HighestShaderModel > D3D_SHADER_MODEL_6_5)
    {
        m_supportedSM = ShaderModel::Sm6_5;
    }
    else
    {
        m_supportedSM = (ShaderModel)(highestSM.HighestShaderModel - D3D_SHADER_MODEL_6_0);
    }

    m_runtimeInfo = DeviceRuntimeInfo{ m_supportedSM };

    if (config.shaderDb)
    {
        m_shaderDb = static_cast<Dx12ShaderDb*>(config.shaderDb);
        CPY_ASSERT_MSG(m_shaderDb->parentDevice() == nullptr, "shader database can only belong to 1 and only 1 device");
        m_shaderDb->setParentDevice(this, &m_runtimeInfo);
    }

    m_gc->start();

    //register counter as a buffer handle
    {
        BufferDesc bufferDesc;
        bufferDesc.format = Format::RGBA_32_UINT;
        bufferDesc.elementCount = 1;
        bufferDesc.memFlags = (MemFlags)0u;
        m_countersBuffer = m_resources->createBuffer(bufferDesc, &m_counterPool->resource());
    }
}

Dx12Device::~Dx12Device()
{
    release(m_countersBuffer);

    m_gc->stop();

    for (auto p : m_dx12WorkInfos->workMap)
        for (auto dl : p.second.downloadMap)
            readbackPool().free(dl.second.memoryBlock);

    delete m_readbackPool;
    delete m_resources;
    delete m_gc;
    delete m_counterPool;
    delete m_queues;
    delete m_descriptors;
    delete m_dx12WorkInfos;

    if (m_device)
        m_device->Release();

    if (m_debugLayer)
        m_debugLayer->Release();

    if (m_computeRootSignature)
        m_computeRootSignature->Release();

    if (m_indirectDispatchCommandSignature)
        m_indirectDispatchCommandSignature->Release();

    if (m_shaderDb && m_shaderDb->parentDevice() == this)
        m_shaderDb->setParentDevice(nullptr, nullptr);

    shutdownCardInfos(g_cardInfos);

}

void Dx12Device::deferRelease(ID3D12Pageable& object)
{
    m_gc->deferRelease(object);
}

TextureResult Dx12Device::createTexture(const TextureDesc& desc)
{
    return m_resources->createTexture(desc);
}

TextureResult Dx12Device::recreateTexture(Texture texture, const TextureDesc& desc)
{
    return m_resources->recreateTexture(texture, desc);
}

BufferResult Dx12Device::createBuffer (const BufferDesc& config)
{
    return m_resources->createBuffer(config);
}

SamplerResult Dx12Device::createSampler (const SamplerDesc& config)
{
    return m_resources->createSampler(config);
}

InResourceTableResult Dx12Device::createInResourceTable(const ResourceTableDesc& config)
{
    return m_resources->createInResourceTable(config);
}

OutResourceTableResult Dx12Device::createOutResourceTable (const ResourceTableDesc& config)
{
    return m_resources->createOutResourceTable(config);
}

SamplerTableResult Dx12Device::createSamplerTable (const ResourceTableDesc& config)
{
    return m_resources->createSamplerTable(config);
}

void Dx12Device::getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo)
{
    m_resources->getResourceMemoryInfo(handle, memInfo);
}

void Dx12Device::release(ResourceHandle resource)
{
    m_resources->release(resource);
}

void Dx12Device::release(ResourceTable table)
{
    m_resources->release(table);
}

WaitStatus Dx12Device::waitOnCpu(WorkHandle handle, int milliseconds)
{
    auto workInfoIt = m_dx12WorkInfos->workMap.find(handle.handleId);
    if (workInfoIt == m_dx12WorkInfos->workMap.end())
        return WaitStatus { WaitErrorType::Invalid, "Invalid work handle." };    

    Dx12Fence& fence = queues().getFence(WorkType::Graphics);

    if (milliseconds != 0u)
        fence.waitOnCpu(workInfoIt->second.fenceValue, milliseconds < 0 ? INFINITE : milliseconds);

    if (fence.isComplete(workInfoIt->second.fenceValue))
        return WaitStatus { WaitErrorType::Ok, "" };
    else
        return WaitStatus { WaitErrorType::NotReady, "" };
}

DownloadStatus Dx12Device::getDownloadStatus(WorkHandle bundle, ResourceHandle handle, int mipLevel, int arraySlice)
{
    auto it = m_dx12WorkInfos->workMap.find(bundle.handleId);
    if (it == m_dx12WorkInfos->workMap.end())
        return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

    ResourceDownloadKey downloadKey { handle, mipLevel, arraySlice };
    auto downloadStateIt = it->second.downloadMap.find(downloadKey);
    if (downloadStateIt == it->second.downloadMap.end())
        return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

    auto& downloadState = downloadStateIt->second;
    Dx12Fence& fence = queues().getFence(downloadState.queueType);
    if (fence.isComplete(downloadState.fenceValue))
    {
        return DownloadStatus {
            DownloadResult::Ok,
            downloadState.memoryBlock.mappedMemory,
            downloadState.memoryBlock.size,
            downloadState.rowPitch,
            downloadState.width,
            downloadState.height,
            downloadState.depth,
        };
    }
    return DownloadStatus { DownloadResult::NotReady, nullptr, 0u };
}

void Dx12Device::enumerate(std::vector<DeviceInfo>& outputList)
{
    CardInfos* cardInfos = &g_cardInfos;
    CardInfos localCardInfos;

    if (cardInfos->refs == 0)
    {
        cacheCardInfos(localCardInfos);
        cardInfos = &localCardInfos;
    }

    int index = 0;
    for (IDXGIAdapter4* card : cardInfos->cards)
    {
        outputList.emplace_back();
        DeviceInfo& outputDeviceInfo = outputList.back();
        outputDeviceInfo.index = index++;
        outputDeviceInfo.valid = true;
        if (card == nullptr)
        {
            outputDeviceInfo.valid = false;
            continue;
        }

        DXGI_ADAPTER_DESC1 desc;
        DX_OK(card->GetDesc1(&desc));

        std::wstring wstr = desc.Description;
        outputDeviceInfo.name = ws2s(wstr);
    }

    if (cardInfos == &localCardInfos)
        shutdownCardInfos(*cardInfos);
}

ScheduleStatus Dx12Device::internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle)
{
    ScheduleStatus status;
    status.workHandle = workHandle;
    
    Dx12WorkBundle dx12WorkBundle(*this);
    {
        m_workDb.lock();
        WorkBundle& workBundle = m_workDb.unsafeGetWorkBundle(workHandle);
        dx12WorkBundle.load(workBundle);
        m_workDb.unlock();
    }

    UINT64 fenceValue = dx12WorkBundle.execute(commandLists, listCounts);

    {
        m_workDb.lock();
        Dx12WorkInfo workInfo;
        workInfo.fenceValue = fenceValue;
        dx12WorkBundle.getDownloadResourceMap(workInfo.downloadMap);
        m_dx12WorkInfos->workMap[workHandle.handleId] = std::move(workInfo);
        m_workDb.unlock();
    }
    return status;
}

void Dx12Device::internalReleaseWorkHandle(WorkHandle handle)
{
    auto workInfoIt = m_dx12WorkInfos->workMap.find(handle.handleId);
    CPY_ASSERT(workInfoIt != m_dx12WorkInfos->workMap.end());
    if (workInfoIt == m_dx12WorkInfos->workMap.end())
        return;

    for (auto downloadStatePair : workInfoIt->second.downloadMap)
        readbackPool().free(downloadStatePair.second.memoryBlock);

    m_dx12WorkInfos->workMap.erase(workInfoIt);
}

SmartPtr<IDisplay> Dx12Device::createDisplay(const DisplayConfig& config)
{
    IDisplay * display = new Dx12Display(config, *this);
    return SmartPtr<IDisplay>(display);
}

Dx12PixApi* Dx12Device::getPixApi() const
{
    return Dx12PixApi::get(m_config.resourcePath.c_str());
}

IDXGIFactory2* Dx12Device::dxgiFactory()
{
    return g_cardInfos.dxgiFactory;
}

}
}

#endif
