#pragma once

#include "Config.h"
#include "Dx12imguiRenderer.h"
#include "Dx12Fence.h"
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include "Win32Window.h"
#include "Dx12Device.h"
#include "Dx12ResourceCollection.h"
#include "Dx12Formats.h"
#include "Dx12Resources.h"
#include "Dx12Display.h"
#include "Dx12Queues.h"
#include "Dx12PixApi.h"
#include "WorkBundleDb.h"
#include <backends/imgui_impl_dx12.h>

namespace coalpy
{
namespace render
{

Dx12imguiRenderer::Dx12imguiRenderer(const IimguiRendererDesc& desc)
: BaseImguiRenderer(desc) 
, m_device(*(Dx12Device*)desc.device)
, m_display(*(Dx12Display*)desc.display)
, m_window(*(Win32Window*)desc.window)
, m_cachedWidth(-1)
, m_cachedHeight(-1)
, m_cachedSwapVersion(-1)
, m_graphicsFence(((Dx12Device*)desc.device)->queues().getFence(WorkType::Graphics))
{
    m_rtv = m_device.descriptors().allocateRtv();

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1 + (int)MaxTextureGpuHandles;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        DX_OK(m_device.device().CreateDescriptorHeap(&desc, DX_RET(m_srvHeap)));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    bool rets2 = ImGui_ImplDX12_Init(
        &m_device.device(),
        m_display.buffering(),
        getDxFormat(m_display.config().format),
        m_srvHeap,
        srvCpuStart,
        srvGpuStart);
    CPY_ASSERT(rets2);

    UINT descriptorIncrSize = m_device.device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //push all free handle slots
    for (int i = 0; i < (int)MaxTextureGpuHandles; ++i)
    {
        UINT descriptorOffset = (1 + i) * descriptorIncrSize;//1 since we reserve the first one for the Init call.
        m_textureGpuHandles[i].ptr = srvGpuStart.ptr + descriptorOffset;
        m_textureCpuHandles[i].ptr = srvCpuStart.ptr + descriptorOffset;
        m_freeGpuHandleIndex.push_back(i);
    }
}

Dx12imguiRenderer::~Dx12imguiRenderer()
{
    m_display.waitForGpu();
    m_device.descriptors().release(m_rtv);

    ImGui_ImplDX12_Shutdown();
}

void Dx12imguiRenderer::newFrame()
{
    BaseImguiRenderer::newFrame();
    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();
}

void Dx12imguiRenderer::setupSwapChain()
{
    auto config = m_display.config();
    if (m_cachedHeight == config.height && m_cachedWidth == config.width && m_cachedSwapVersion == m_display.version())
        return;

    m_device.resources().lock();
    Texture t = m_display.texture();
    Dx12Resource& r = m_device.resources().unsafeGetResource(t);
    m_device.device().CreateRenderTargetView(&r.d3dResource(), nullptr, m_rtv.handle);
    m_device.resources().unlock();

    m_cachedWidth = config.width;
    m_cachedHeight = config.height;
    m_cachedSwapVersion = m_display.version();
}

void Dx12imguiRenderer::render()
{
    flushPendingDeleteIndices();
    setupSwapChain();
    activate();

    ImGui::Render();
    auto workType = WorkType::Graphics;
    Dx12List dx12List;
    m_device.queues().allocate(workType, dx12List);
    Dx12PixApi* pixApi = m_device.getPixApi();
    if (pixApi)
        pixApi->pixBeginEventOnCommandList(dx12List.list, 0xffffffff, "Dx12imguiRenderer::render");
    
    //All resources required to be SRVS, and their barriers to put them back where they started.
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    {
        barriers.reserve(1 + m_texToGpuHandleIndex.size());
        Texture targetTexture = m_display.texture();
        
        m_device.transitionResourceState(targetTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers);

        for (auto it : m_texToGpuHandleIndex)
            m_device.transitionResourceState(it.first, getDx12GpuState(ResourceGpuState::Srv), barriers);
    }

    dx12List.list->ResourceBarrier((UINT)barriers.size(), barriers.data());
    dx12List.list->OMSetRenderTargets(1, &m_rtv.handle, false, nullptr);
    
    ID3D12DescriptorHeap* srvHeap = &(*m_srvHeap);
    dx12List.list->SetDescriptorHeaps(1, &srvHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &(*dx12List.list));

    if (pixApi)
        pixApi->pixEndEventOnCommandList(dx12List.list);

    dx12List.list->Close();

    ID3D12CommandList* cmdList = &(*dx12List.list);
    m_device.queues().cmdQueue(workType).ExecuteCommandLists(1, &cmdList);
    auto fenceVal = m_device.queues().signalFence(workType);
    m_device.queues().deallocate(dx12List, fenceVal);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

ImTextureID Dx12imguiRenderer::registerTexture(Texture texture)
{
    auto it = m_texToGpuHandleIndex.find(texture);
    int index = -1;
    if (it != m_texToGpuHandleIndex.end())
    {
        index = it->second;
    }
    else if(!m_freeGpuHandleIndex.empty())
    {
        m_device.resources().lock();
        Dx12Resource& resource = m_device.resources().unsafeGetResource(texture);
        if (!resource.isBuffer())
        {
            auto& textureResource = (Dx12Texture&)resource;
            if ((textureResource.config().memFlags & MemFlag_GpuRead) != 0)
            {
                index = m_freeGpuHandleIndex.back();
                m_freeGpuHandleIndex.pop_back();
                m_texToGpuHandleIndex[texture] = index;

                Dx12Descriptor descriptor = textureResource.srv();
                m_device.device().CopyDescriptorsSimple(
                    1, m_textureCpuHandles[index], descriptor.handle,
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }
        m_device.resources().unlock();
    }
    else
    {
        CPY_ERROR_FMT(false, "Reached max limit of imgui textures registered. Can only allow up to %d textures to be used in the Imgui.", (int)MaxTextureGpuHandles);
    }

    return index >= 0 ? (ImTextureID)m_textureGpuHandles[index].ptr : (ImTextureID)nullptr;
}

void Dx12imguiRenderer::flushPendingDeleteIndices()
{
    while (!m_textureDeleteQueue.empty())
    {
        PendingFreeIndex& obj = m_textureDeleteQueue.front();
        if (!m_graphicsFence.isComplete(obj.fenceVal))
            break;

        m_freeGpuHandleIndex.push_back(obj.gpuHandleIndex);
        m_textureDeleteQueue.pop();
    }
}

bool Dx12imguiRenderer::isTextureRegistered(Texture texture) const
{
    return m_texToGpuHandleIndex.find(texture) != m_texToGpuHandleIndex.end();
}

void Dx12imguiRenderer::unregisterTexture(Texture texture)
{
    auto it = m_texToGpuHandleIndex.find(texture);
    CPY_ASSERT(it != m_texToGpuHandleIndex.end());
    if (it == m_texToGpuHandleIndex.end())
        return;

    PendingFreeIndex pendingFreeIndex { m_graphicsFence.signal(), it->second };
    m_textureDeleteQueue.push(pendingFreeIndex);
    m_texToGpuHandleIndex.erase(texture);
}

}
}
