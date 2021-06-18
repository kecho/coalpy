#include <Config.h>

#if ENABLE_DX12

#include <coalpy.core/Assert.h>
#include "Dx12Display.h"
#include "Dx12Device.h"
#include "Dx12Formats.h"
#include <D3D12.h>
#include <dxgi1_5.h>

namespace coalpy
{
namespace render
{

Dx12Display::Dx12Display(const DisplayConfig& config, Dx12Device& device)
: IDisplay(config), m_device(device), m_swapChain(nullptr)
{
    IDXGIFactory2* factory = Dx12Device::dxgiFactory();
    CPY_ASSERT(factory != nullptr);
    if (!factory)
        return;

    SmartPtr<IDXGIFactory4> factory4;
    DX_OK(factory->QueryInterface((IDXGIFactory4**)&factory4));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

    // Set the width and height of the back buffer.
    swapChainDesc.Width = config.width;
    swapChainDesc.Height = config.height;
    
    // Set regular 32-bit surface for the back buffer.
    swapChainDesc.Format = getDxFormat(config.format);
    swapChainDesc.Stereo = FALSE;
    
    // Turn multisampling off.
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = config.buffering;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    
    // Discard the back buffer contents after presenting.
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    // Don't set the advanced flags.
    //swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.Flags = 0;
    
    SmartPtr<IDXGISwapChain1> swapChain1;
    
    DX_OK(factory->CreateSwapChainForHwnd(
        /*m_device.GetQueueManager()->GetDirect()*/nullptr,
        (HWND)config.handle,
        &swapChainDesc,
        NULL, //no fullscreen desc
        NULL,
        (IDXGISwapChain1**)&swapChain1));
    
    DX_OK(swapChain1->QueryInterface(&m_swapChain));
    
#if 0
    TextureConfig surfaceConfig; 
    surfaceConfig.type = TextureType_2d;
    surfaceConfig.width = config.width;
    surfaceConfig.height = config.height;
    surfaceConfig.depth = 1;
    surfaceConfig.mipLevels = 1;
    surfaceConfig.name = "DisplayBuffer";
    surfaceConfig.format = config.format;
    surfaceConfig.bindFlags = BindFlags_Rt;
    surfaceConfig.usage = ResourceUsage_Static;
    
    for (int i = 0; i < (int)mConfig.buffering; ++i)
    {
        ID3D12Resource* resource = nullptr;
        //Record RTVs
        DX_VALID(mSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource)));
    
        Dx12TextureRef t = D12_NEW(parentDevice->GetAllocator(), "DisplayBufferAlloc") Dx12Texture(surfaceConfig, parentDevice);
        t->AcquireD3D12Resource(resource);
        t->init();
        mTextures.push_back(t);
    
        RenderTargetConfig rtConfig;
        rtConfig.colorCount = 1u;
        rtConfig.colors[0] = &(*t);
        Dx12RenderTargetRef rt = D12_NEW(parentDevice->GetAllocator(), "DisplayBufferRt") Dx12RenderTarget(rtConfig, parentDevice);
        mRts.push_back(rt);
    
        mFenceVals.push_back(0ull);
    }
#endif
}


void Dx12Display::resize(unsigned int width, unsigned int height)
{
}

Texture Dx12Display::texture()
{
    return Texture();
}

Dx12Display::~Dx12Display()
{
    if (m_swapChain)
        m_swapChain->Release();
}

}
}

#endif
