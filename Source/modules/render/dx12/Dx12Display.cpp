#include <Config.h>

#if ENABLE_DX12

#include <coalpy.core/Assert.h>
#include "Dx12Display.h"
#include "Dx12Device.h"
#include "Dx12Queues.h"
#include "Dx12Formats.h"
#include "Dx12Fence.h"
#include "Dx12ResourceCollection.h"
#include "WorkBundleDb.h"
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
        &m_device.queues().cmdQueue(WorkType::Graphics),
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
    // Piece of shit dx12 does not allow UAV access to swap chain. See doRetardedDXGIDx12Hack
    //m_surfaceDesc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
    m_surfaceDesc.memFlags = {};
    m_buffering = config.buffering;
    
    acquireTextures();
    m_fenceVals.resize(m_buffering, 0);
}

void Dx12Display::createComputeTexture()
{
    TextureDesc desc = m_surfaceDesc;
    desc.name = "computeBackbuffer";
    desc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
    desc.isRtv = true;
    desc.recreatable = true;
    if (!m_computeTexture.valid())
    {
        TextureResult result = m_device.resources().createTexture(desc, nullptr);
        CPY_ASSERT_FMT(result.success(), "Could not create swap chain compute texture: %s", result.message.c_str());
        m_computeTexture = result.texture;
    }
    else
    {
        TextureResult result = m_device.resources().recreateTexture(m_computeTexture, desc);
        CPY_ASSERT_FMT(result.success(), "Could not create swap chain compute texture: %s", result.message.c_str());
    }

    m_copyCmdLists.resize(m_buffering);
    for (int i = 0; i < m_buffering; ++i)
    {
        CommandList& list = m_copyCmdLists[i];
        list.reset();
        CopyCommand copyCmd;
        copyCmd.setResources(m_computeTexture, m_textures[i]);
        list.writeCommand(copyCmd);
        list.finalize();
    }
}

void Dx12Display::acquireTextures()
{
    for (int i = 0; i < m_buffering; ++i)
    {
        SmartPtr<ID3D12Resource> resource;
        DX_OK(m_swapChain->GetBuffer(i, DX_RET(resource)));
        TextureResult texResult = m_device.resources().createTexture(m_surfaceDesc, resource, ResourceSpecialFlag_NoDeferDelete);
        CPY_ASSERT_FMT(texResult.success(), "Could not create swap chain texture, error: %s", texResult.message.c_str());
        m_textures.push_back(texResult.texture);
    }
    createComputeTexture();
    ++m_version;
}

void Dx12Display::waitForGpu()
{
    Dx12Fence& f = m_device.queues().getFence(WorkType::Graphics);
    auto s = m_device.queues().signalFence(WorkType::Graphics);
    f.waitOnCpu(s);
}

void Dx12Display::resize(unsigned int width, unsigned int height)
{
    waitForGpu();

    for (auto t : m_textures)
        m_device.release(t);

    m_textures.clear();

    DX_OK(m_swapChain->ResizeBuffers(m_config.buffering, width, height, getDxFormat(m_config.format), 0));

    m_config.width = width;
    m_config.height = height;
    m_surfaceDesc.width  = width;
    m_surfaceDesc.height = height;
    acquireTextures();
}

Texture Dx12Display::texture()
{
    return m_computeTexture;
    //return m_textures[m_swapChain->GetCurrentBackBufferIndex()];
}

UINT64 Dx12Display::fenceVal() const
{
    return m_fenceVals[m_swapChain->GetCurrentBackBufferIndex()];
}

void Dx12Display::present()
{
    Dx12Fence& fence = m_device.queues().getFence(WorkType::Graphics);
    present(fence);
}

void Dx12Display::doRetardedDXGIDx12Hack(int bufferIndex)
{
    //This is an absolute abomination. So the TL;DR is that for whatever the fuck reason,
    //microsoft does not support write UAV on the swap chain. I've spoken with folks from AMD
    //and they are as baffled as I am. 
    //So what im doing is that im breaking all my careful design to accomodate for this issue.
    //I have 1 single resource which is a uav, where compute can write, then i shove in a command list
    //that copies to the swap table.
    //To avoid exposing the concept of present all the way up to the high level, just using my intermediate API
    //utilities to submit / manage the resource state and submit a copy into the swap chain.
    // Thank you microsoft.    

    auto& workDb = m_device.workDb();
    auto presentTexture = m_textures[bufferIndex];
    workDb.lock();
    auto workType = WorkType::Graphics;
    Dx12List lists;
    m_device.queues().allocate(workType, lists);
    Dx12Resource& srcRes = m_device.resources().unsafeGetResource(m_computeTexture);
    Dx12Resource& dstRes = m_device.resources().unsafeGetResource(presentTexture);

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    {
        auto srcSrcState = getDx12GpuState(workDb.resourceInfos()[m_computeTexture].gpuState);
        auto dstSrcState = D3D12_RESOURCE_STATE_COPY_SOURCE;
        if (srcSrcState != dstSrcState)
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource = &srcRes.d3dResource();
            b.Transition.StateBefore = srcSrcState;
            b.Transition.StateAfter = dstSrcState;
            workDb.resourceInfos()[m_computeTexture].gpuState = getGpuState(dstSrcState);
            barriers.push_back(b);
        }

        if (srcSrcState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            b.UAV.pResource = &srcRes.d3dResource();;
            barriers.push_back(b);
        }
    }
    {
        auto dstSrcState = getDx12GpuState(workDb.resourceInfos()[presentTexture].gpuState);
        auto dstDstState = D3D12_RESOURCE_STATE_COPY_DEST;
        if (dstSrcState != dstDstState)
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource = &dstRes.d3dResource();
            b.Transition.StateBefore = dstSrcState;
            b.Transition.StateAfter = dstDstState;
            workDb.resourceInfos()[presentTexture].gpuState = getGpuState(dstDstState);
            barriers.push_back(b);
        }
    }

    if (!barriers.empty())
        lists.list->ResourceBarrier(barriers.size(), barriers.data());

    lists.list->CopyResource(&dstRes.d3dResource(), &srcRes.d3dResource());
    D3D12_RESOURCE_BARRIER presentBarrier = {};
    presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    presentBarrier.Transition.pResource = &dstRes.d3dResource();
    presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    workDb.resourceInfos()[presentTexture].gpuState = getGpuState(D3D12_RESOURCE_STATE_PRESENT);
    lists.list->ResourceBarrier(1, &presentBarrier);
    lists.list->Close();
    auto& queue = m_device.queues().cmdQueue(workType);
    ID3D12CommandList* list = &(*lists.list);
    queue.ExecuteCommandLists(1, &list);
    auto fenceVal = m_device.queues().signalFence(workType);
    m_device.queues().deallocate(lists, fenceVal);
    workDb.unlock();
}

int Dx12Display::currentBuffer() const
{
    return (int)m_swapChain->GetCurrentBackBufferIndex();
}

void Dx12Display::present(Dx12Fence& fence)
{
    auto bufferIndex = currentBuffer();

    doRetardedDXGIDx12Hack(bufferIndex);
    auto res = m_swapChain->Present(1u, 0u);
    CPY_ASSERT(res == S_OK || res == DXGI_STATUS_OCCLUDED);

    UINT64 nextBufferIndex = (bufferIndex + 1) % m_buffering;
    if (!fence.isComplete(m_fenceVals[nextBufferIndex]))
        fence.waitOnCpu(m_fenceVals[nextBufferIndex]);
    m_fenceVals[bufferIndex] = fence.signal();
}

Dx12Display::~Dx12Display()
{
    waitForGpu();

    for (auto t : m_textures)
        m_device.resources().release(t);

    m_textures.clear();

    if (m_computeTexture.valid())
        m_device.release(m_computeTexture);
    m_computeTexture = Texture();

    if (m_swapChain)
        m_swapChain->Release();
}

}
}

#endif
