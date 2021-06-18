#include <Config.h>

#if ENABLE_DX12

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

#if _DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid")
#endif

#include <dxgi1_6.h>

#include "Dx12Device.h"
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <vector>

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
} g_CardInfos;

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
{
    cacheCardInfos(g_CardInfos);
}

Dx12Device::~Dx12Device()
{
    shutdownCardInfos(g_CardInfos);
}

Texture Dx12Device::platCreateTexture(const TextureDesc& desc)
{
    return Texture();
}

void Dx12Device::enumerate(std::vector<DeviceInfo>& outputList)
{
    CardInfos* cardInfos = &g_CardInfos;
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

}
}

#endif
