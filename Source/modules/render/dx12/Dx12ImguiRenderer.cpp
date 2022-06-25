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
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImGui
{
extern ImGuiContext* ImGui::CreateContext(ImFontAtlas* shared_font_atlas);
}

namespace coalpy
{
namespace render
{

Dx12imguiRenderer::Dx12imguiRenderer(const IimguiRendererDesc& desc)
: m_device(*(Dx12Device*)desc.device)
, m_display(*(Dx12Display*)desc.display)
, m_window(*(Win32Window*)desc.window)
, m_cachedWidth(-1)
, m_cachedHeight(-1)
, m_cachedSwapVersion(-1)
, m_graphicsFence(((Dx12Device*)desc.device)->queues().getFence(WorkType::Graphics))
{
    auto oldContext = ImGui::GetCurrentContext();
    auto oldPlotContext = ImPlot::GetCurrentContext();
    m_context = ImGui::CreateContext();
    m_plotContext = ImPlot::CreateContext();
    ImGui::SetCurrentContext(m_context);
    ImPlot::SetCurrentContext(m_plotContext);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    m_rtv = m_device.descriptors().allocateRtv();

    m_windowHookId = m_window.addHook(
        [this](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
        {
            return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        });

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1 + (int)MaxTextureGpuHandles;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        DX_OK(m_device.device().CreateDescriptorHeap(&desc, DX_RET(m_srvHeap)));
    }

    bool rets1 = ImGui_ImplWin32_Init((HWND)m_window.getHandle());
    CPY_ASSERT(rets1);

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

    setCoalpyStyle();

    ImGui::SetCurrentContext(oldContext);
    ImPlot::SetCurrentContext(oldPlotContext);

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
    m_window.removeHook(m_windowHookId);
    m_device.descriptors().release(m_rtv);

    ImGui::SetCurrentContext(m_context);
    ImPlot::SetCurrentContext(m_plotContext);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void Dx12imguiRenderer::activate()
{
    ImGui::SetCurrentContext(m_context);
    ImPlot::SetCurrentContext(m_plotContext);
}

void Dx12imguiRenderer::newFrame()
{
    activate();
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Dx12imguiRenderer::endFrame()
{
    activate();
}

void Dx12imguiRenderer::setCoalpyStyle()
{
    /* Green/Dark theme in ImGuiFontStudio */
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.44f, 0.44f, 0.44f, 0.60f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.57f, 0.57f, 0.57f, 0.70f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.76f, 0.76f, 0.76f, 0.80f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
    colors[ImGuiCol_Button]                 = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
    colors[ImGuiCol_Header]                 = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.36f, 0.36f, 0.36f, 0.54f);
    //colors[ImGuiCol_DockingPreview]         = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
    //colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.13f, 0.13f, 0.13f, 0.80f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

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

void Dx12imguiRenderer::transitionResourceState(ResourceHandle resource, D3D12_RESOURCE_STATES newState, std::vector<D3D12_RESOURCE_BARRIER>& outBarriers)
{
    auto& workDb = m_device.workDb();
    WorkResourceInfo& resourceInfo = workDb.resourceInfos()[resource];
    auto stateBefore = getDx12GpuState(resourceInfo.gpuState);
    if (stateBefore == newState)
        return;

    outBarriers.emplace_back();
    D3D12_RESOURCE_BARRIER& b = outBarriers.back();
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    b.Transition.pResource = &m_device.resources().unsafeGetResource(resource).d3dResource();
    b.Transition.StateBefore = getDx12GpuState(resourceInfo.gpuState);
    b.Transition.StateAfter = newState;
    resourceInfo.gpuState = getGpuState(newState);
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
        transitionResourceState(targetTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers);

        for (auto it : m_texToGpuHandleIndex)
            transitionResourceState(it.first, getDx12GpuState(ResourceGpuState::Srv), barriers);
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
