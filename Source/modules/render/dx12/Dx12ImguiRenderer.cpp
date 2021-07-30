#pragma once

#include "Config.h"
#include "Dx12imguiRenderer.h"
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include "Win32Window.h"
#include "Dx12Device.h"
#include "Dx12ResourceCollection.h"
#include "Dx12Formats.h"
#include "Dx12Resources.h"
#include "Dx12Display.h"
#include "Dx12Queues.h"
#include "WorkBundleDb.h"
#include <imgui.h>
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
{
    m_context = ImGui::CreateContext();

    m_rtv = m_device.descriptors().allocateRtv();

    m_windowHookId = m_window.addHook(
        [this](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
        {
            return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        });

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        DX_OK(m_device.device().CreateDescriptorHeap(&desc, DX_RET(m_srvHeap)));
    }

    bool rets1 = ImGui_ImplWin32_Init((HWND)m_window.getHandle());
    CPY_ASSERT(rets1);

    bool rets2 = ImGui_ImplDX12_Init(
        &m_device.device(),
        m_display.buffering(),
        getDxFormat(m_display.config().format),
        m_srvHeap,
        m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_srvHeap->GetGPUDescriptorHandleForHeapStart());
    CPY_ASSERT(rets2);

    setCoalpyStyle();
}

Dx12imguiRenderer::~Dx12imguiRenderer()
{
    m_display.waitForGpu();
    m_window.removeHook(m_windowHookId);
    m_device.descriptors().release(m_rtv);

    ImGui::SetCurrentContext(m_context);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Dx12imguiRenderer::activate()
{
    ImGui::SetCurrentContext(m_context);
}

void Dx12imguiRenderer::newFrame()
{
    activate();
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
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
    if (m_cachedHeight == config.height && m_cachedWidth == config.width)
        return;

    m_device.resources().lock();
    Texture t = m_display.texture();
    Dx12Resource& r = m_device.resources().unsafeGetResource(t);
    m_device.device().CreateRenderTargetView(&r.d3dResource(), nullptr, m_rtv.handle);
    m_device.resources().unlock();

    m_cachedWidth = config.width;
    m_cachedHeight = config.height;
}

void Dx12imguiRenderer::render()
{
    setupSwapChain();
    activate();

    ImGui::Render();
    auto workType = WorkType::Graphics;
    Dx12List dx12List;
    m_device.queues().allocate(workType, dx12List);
    
    auto& workDb = m_device.workDb();

    {
        Texture targetTexture = m_display.texture();
        WorkResourceInfo& textureResourceInfo = workDb.resourceInfos()[targetTexture];
        auto stateBefore = getDx12GpuState(textureResourceInfo.gpuState);
        if (stateBefore != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            b.Transition.pResource = &m_device.resources().unsafeGetResource(targetTexture).d3dResource();
            b.Transition.StateBefore = getDx12GpuState(textureResourceInfo.gpuState);
            b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            textureResourceInfo.gpuState = getGpuState(D3D12_RESOURCE_STATE_RENDER_TARGET);
            dx12List.list->ResourceBarrier(1, &b);
        }
    }

    dx12List.list->OMSetRenderTargets(1, &m_rtv.handle, false, nullptr);
    
    ID3D12DescriptorHeap* srvHeap = &(*m_srvHeap);
    dx12List.list->SetDescriptorHeaps(1, &srvHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &(*dx12List.list));
    dx12List.list->Close();

    ID3D12CommandList* cmdList = &(*dx12List.list);
    m_device.queues().cmdQueue(workType).ExecuteCommandLists(1, &cmdList);
    auto fenceVal = m_device.queues().signalFence(workType);
    m_device.queues().deallocate(dx12List, fenceVal);
}

IimguiRenderer* IimguiRenderer::create(const IimguiRendererDesc& desc)
{
    if (desc.device == nullptr || desc.window == nullptr || desc.display == nullptr)
    {
        CPY_ERROR_MSG(false, "Invalid arguments for IimguiRenderer.");
        return nullptr;
    }

    return new Dx12imguiRenderer(desc);
}


}
}
