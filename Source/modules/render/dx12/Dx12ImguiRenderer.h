#pragma once

#include <d3d12.h>
#include <coalpy.render/IimguiRenderer.h>
#include "Dx12DescriptorPool.h" 
#include <imgui.h>

namespace coalpy
{

class Win32Window;

namespace render
{

class Dx12Device;
class Dx12Display;
class Dx12Texture;

class Dx12imguiRenderer : public IimguiRenderer
{
public:
    Dx12imguiRenderer(const IimguiRendererDesc& desc);
    virtual ~Dx12imguiRenderer();

    virtual void newFrame() override;
    virtual void activate() override;
    virtual void render() override;

private:

    void setupSwapChain();

    Dx12Device& m_device;
    Dx12Display& m_display;
    Win32Window& m_window;
    int m_windowHookId;
    int m_cachedWidth;
    int m_cachedHeight;

    Dx12Descriptor m_rtv;
    SmartPtr<ID3D12DescriptorHeap> m_srvHeap;

    ImGuiContext* m_context;
};

}
}
