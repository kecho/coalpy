#pragma once
#include <coalpy.render/Resources.h>
#include <coalpy.render/IDisplay.h>
#include <vector>
#include <d3d12.h>

struct IDXGISwapChain4;

namespace coalpy
{
namespace render
{

class Dx12Device;
class Dx12Fence;

class Dx12Display : public IDisplay
{
public:
    Dx12Display(const DisplayConfig& config, Dx12Device& device);
    virtual ~Dx12Display();
    virtual Texture texture() override;
    virtual void resize(unsigned int width, unsigned int height) override;
    void acquireTextures();

    UINT64 fenceVal() const;
    void present(Dx12Fence& fence);

private:
    TextureDesc m_surfaceDesc;
    int m_buffering = 0;
    std::vector<ID3D12Resource*> m_resources;
    std::vector<Texture> m_textures;
    std::vector<UINT64> m_fenceVals;
    IDXGISwapChain4* m_swapChain;
    Dx12Device& m_device;
    
};

}
}
