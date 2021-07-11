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
#include <vector>

#include "Dx12Device.h"
#include "Dx12Queues.h"
#include "Dx12Display.h"
#include "Dx12ShaderDb.h"
#include "Dx12ResourceCollection.h"
#include "Dx12DescriptorPool.h"
#include "Dx12WorkBundle.h"
#include "Dx12Fence.h"

namespace coalpy
{
namespace render
{

namespace 
{

struct CardInfos
{
    bool initialized = false;
    int  refs = 0;
    SmartPtr<IDXGIFactory2> dxgiFactory;
    std::vector<SmartPtr<IDXGIAdapter4>> cards;
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
        cardInfos.cards.push_back(adapter4);
    }
}

void shutdownCardInfos(CardInfos& cardInfos)
{
    --cardInfos.refs;
    if (cardInfos.refs == 0)
        cardInfos = CardInfos();
}

ID3D12RootSignature* createComputeRootSignature(ID3D12Device2& device)
{
    static const int totalNumberTables = (int)Dx12Device::MaxNumTables * (int)Dx12Device::TableTypes::Count;
    D3D12_ROOT_PARAMETER1 rootParams[totalNumberTables];
    D3D12_DESCRIPTOR_RANGE1 ranges[totalNumberTables];

    static const D3D12_DESCRIPTOR_RANGE_TYPE g_rangeTypes[(int)Dx12Device::TableTypes::Count] = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
    };

    for (int tableTypeId = 0; tableTypeId < (int)Dx12Device::TableTypes::Count; ++tableTypeId)
    {
        for (int tableId = 0; tableId < (int)Dx12Device::MaxNumTables; ++tableId)
        {
            auto tableType = (Dx12Device::TableTypes)tableTypeId;
            int tableIndex = tableTypeId * (int)Dx12Device::MaxNumTables + tableId;
            auto& rootParam = rootParams[tableIndex];
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

            rootParam.DescriptorTable.NumDescriptorRanges = 1;
            {
                auto& typeRange = ranges[tableIndex];
                typeRange.RangeType = g_rangeTypes[tableTypeId];
                typeRange.NumDescriptors = UINT_MAX;
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
{
    m_info = {};
    m_dx12WorkInfos = new Dx12WorkInformationMap;
    cacheCardInfos(g_cardInfos);

#if _DEBUG
    DX_OK(D3D12GetDebugInterface(DX_RET(m_debugLayer)));
    if (m_debugLayer)
        m_debugLayer->EnableDebugLayer();
#endif

    if (g_cardInfos.cards.empty())
        return;

    int selectedDeviceIdx = std::min<int>(std::max<int>(config.index, 0), (int)g_cardInfos.cards.size() - 1);
    SmartPtr<IDXGIAdapter4>& selectedCard = g_cardInfos.cards[selectedDeviceIdx];
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
    m_resources = new Dx12ResourceCollection(*this, m_workDb);

    m_computeRootSignature = createComputeRootSignature(*m_device);

    if (config.shaderDb)
    {
        m_shaderDb = static_cast<Dx12ShaderDb*>(config.shaderDb);
        CPY_ASSERT_MSG(m_shaderDb->parentDevice() == nullptr, "shader database can only belong to 1 and only 1 device");
        m_shaderDb->setParentDevice(this);
    }
}

Dx12Device::~Dx12Device()
{
    delete m_resources;
    delete m_queues;
    delete m_descriptors;
    delete m_dx12WorkInfos;

    if (m_device)
        m_device->Release();

    if (m_debugLayer)
        m_debugLayer->Release();

    if (m_computeRootSignature)
        m_computeRootSignature->Release();

    if (m_shaderDb && m_shaderDb->parentDevice() == this)
        m_shaderDb->setParentDevice(nullptr);

    shutdownCardInfos(g_cardInfos);
}

Texture Dx12Device::createTexture(const TextureDesc& desc)
{
    return m_resources->createTexture(desc);
}

Buffer Dx12Device::createBuffer (const BufferDesc& config)
{
    return m_resources->createBuffer(config);
}

InResourceTable Dx12Device::createInResourceTable(const ResourceTableDesc& config)
{
    return m_resources->createInResourceTable(config);;
}

OutResourceTable Dx12Device::createOutResourceTable (const ResourceTableDesc& config)
{
    return m_resources->createOutResourceTable(config);
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

DownloadStatus Dx12Device::getDownloadStatus(WorkHandle bundle, ResourceHandle handle)
{
    auto it = m_dx12WorkInfos->workMap.find(bundle.handleId);
    if (it == m_dx12WorkInfos->workMap.end())
        return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

    auto downloadStateIt = it->second.downloadMap.find(handle);
    if (downloadStateIt == it->second.downloadMap.end())
        return DownloadStatus { DownloadResult::Invalid, nullptr, 0u };

    auto& downloadState = downloadStateIt->second;
    Dx12Fence& fence = queues().getFence(downloadState.queueType);
    if (fence.isComplete(downloadState.fenceValue))
    {
        return DownloadStatus { DownloadResult::Ok, downloadState.mappedMemory, 0u };
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
    for (SmartPtr<IDXGIAdapter4>& card : cardInfos->cards)
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
    m_dx12WorkInfos->workMap.erase(handle.handleId);
}

SmartPtr<IDisplay> Dx12Device::createDisplay(const DisplayConfig& config)
{
    IDisplay * display = new Dx12Display(config, *this);
    return SmartPtr<IDisplay>(display);
}

IDXGIFactory2* Dx12Device::dxgiFactory()
{
    return g_cardInfos.dxgiFactory;
}

}
}

#endif
