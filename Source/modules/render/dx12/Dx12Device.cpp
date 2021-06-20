#include <Config.h>

#if ENABLE_DX12

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

#if _DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid")
#endif

#include <dxgi1_6.h>
#include <D3D12.h>

#include "Dx12Device.h"
#include "Dx12Queues.h"
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <vector>
#include <algorithm>
#include "Dx12Display.h"
#include "Dx12ResourceCollection.h"

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

}

Dx12Device::Dx12Device(const DeviceConfig& config)
: TDevice<Dx12Device>(config)
, m_debugLayer(nullptr)
, m_device(nullptr)
{
    m_info = {};
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

    m_queues = new Dx12Queues(*this);
    m_resources = new Dx12ResourceCollection(*this);
}

Dx12Device::~Dx12Device()
{
    delete m_queues;
    delete m_resources;

    if (m_device)
        m_device->Release();

    if (m_debugLayer)
        m_debugLayer->Release();

    shutdownCardInfos(g_cardInfos);
}

Texture Dx12Device::createTexture(const TextureDesc& desc)
{
    return Texture();
}

Buffer Dx12Device::createBuffer (const BufferDesc& config)
{
    return Buffer();
}

InResourceTable Dx12Device::createInResourceTable(const ResourceTableDesc& config)
{
    return InResourceTable();
}

OutResourceTable Dx12Device::createOutResourceTable (const ResourceTableDesc& config)
{
    return OutResourceTable();
}

void Dx12Device::release(ResourceHandle resource)
{
}

void Dx12Device::release(ResourceTable table)
{
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
