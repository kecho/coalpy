#include <Config.h>

#if ENABLE_DX12

#include <coalpy.core/Assert.h>
#include "Dx12Display.h"
#include "Dx12Device.h"
#include "Dx12Queues.h"
#include "Dx12Formats.h"
#include "Dx12Fence.h"
#include "Dx12ResourceCollection.h"
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
        &m_device.queues().directq(),
        (HWND)config.handle,
        &swapChainDesc,
        NULL, //no fullscreen desc
        NULL,
        (IDXGISwapChain1**)&swapChain1));
    
    DX_OK(swapChain1->QueryInterface(&m_swapChain));
    
    m_surfaceDesc.type = TextureType::k2d;
    m_surfaceDesc.width = config.width;
    m_surfaceDesc.height = config.height;
    m_surfaceDesc.depth = 1;
    m_surfaceDesc.mipLevels = 1;
    m_surfaceDesc.name = "DisplayBuffer";
    m_surfaceDesc.format = config.format;
    // Piece of shit dx12 does not allow UAV access to swap chain. Why microsoft?
    //m_surfaceDesc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
    m_buffering = config.buffering;
    
    acquireTextures();
    m_fenceVals.resize(m_buffering, 0);
}


void Dx12Display::acquireTextures()
{
    for (int i = 0; i < m_buffering; ++i)
    {
        ID3D12Resource* resource;
        DX_OK(m_swapChain->GetBuffer(i, DX_RET(resource)));
        Texture t = m_device.resources().createTexture(m_surfaceDesc, resource);
        m_resources.push_back(resource);
        m_textures.push_back(t);
    }
}


void Dx12Display::resize(unsigned int width, unsigned int height)
{
    for (auto t : m_textures)
        m_device.release(t);

    m_textures.clear();

    for (auto r : m_resources)
        r->Release();
    m_resources.clear();

    DX_OK(m_swapChain->ResizeBuffers(m_config.buffering, width, height, getDxFormat(m_config.format), 0));

    m_surfaceDesc.width  = width;
    m_surfaceDesc.height = height;
    acquireTextures();
}

Texture Dx12Display::texture()
{
    return m_textures[m_swapChain->GetCurrentBackBufferIndex()];
}

UINT64 Dx12Display::fenceVal() const
{
    return m_fenceVals[m_swapChain->GetCurrentBackBufferIndex()];
}

void Dx12Display::present(Dx12Fence& fence)
{
    auto bufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    DX_OK(m_swapChain->Present(1u, 0u));
    m_fenceVals[bufferIndex] = fence.signal();
}

Dx12Display::~Dx12Display()
{
    for (auto t : m_textures)
        m_device.resources().release(t);

    if (m_swapChain)
        m_swapChain->Release();
}

}
}

#endif
