#pragma once

#include <d3d12.h>
#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.render/Resources.h>
#include "Dx12DescriptorPool.h" 
#include <imgui.h>
#include <vector>
#include <queue>
#include <unordered_map>

namespace coalpy
{

class Win32Window;

namespace render
{

class Dx12Device;
class Dx12Display;
class Dx12Texture;
class Dx12Fence;
struct Dx12List;

class Dx12imguiRenderer : public IimguiRenderer
{
public:
    Dx12imguiRenderer(const IimguiRendererDesc& desc);
    virtual ~Dx12imguiRenderer();

    virtual void newFrame() override;
    virtual void activate() override;
    virtual void render() override;
    ImTextureID registerTexture(Texture texture) override;
    void unregisterTexture(Texture texture) override;

private:
    enum 
    {
        MaxTextureGpuHandles = 64
    };

    void setCoalpyStyle();
    void setupSwapChain();
    void flushPendingDeleteIndices();
    void transitionResourceState(ResourceHandle resource, D3D12_RESOURCE_STATES newState, std::vector<D3D12_RESOURCE_BARRIER>& outBarriers);

    Dx12Fence& m_graphicsFence;
    Dx12Device& m_device;
    Dx12Display& m_display;
    Win32Window& m_window;
    int m_windowHookId;
    int m_cachedWidth;
    int m_cachedHeight;
    int m_cachedSwapVersion;

    Dx12Descriptor m_rtv;
    SmartPtr<ID3D12DescriptorHeap> m_srvHeap;

    std::unordered_map<Texture, int> m_texToGpuHandleIndex;
    D3D12_GPU_DESCRIPTOR_HANDLE m_textureGpuHandles[MaxTextureGpuHandles];
    D3D12_CPU_DESCRIPTOR_HANDLE m_textureCpuHandles[MaxTextureGpuHandles];

    struct PendingFreeIndex
    {
        UINT64 fenceVal;
        int gpuHandleIndex;
    };

    std::vector<int> m_freeGpuHandleIndex;
    ImGuiContext* m_context;

    std::queue<PendingFreeIndex> m_textureDeleteQueue;
};

}
}
