#pragma once
#include <coalpy.render/Resources.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.core/SmartPtr.h>
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
    virtual void present() override;
    void acquireTextures();

    int buffering() const { return m_buffering; }
    Texture getTexture(int index) { return m_textures[index]; }
    int currentBuffer() const;

    UINT64 fenceVal() const;

    void waitForGpu();
    void present(Dx12Fence& fence);

    int version() { return m_version; }
private:
    void copyToSwapChain(int bufferIndex);
    void createComputeTexture();
    TextureDesc m_surfaceDesc;
    int m_version = 0;
    int m_buffering = 0;
    Texture m_computeTexture;
    std::vector<Texture> m_textures;
    std::vector<UINT64>  m_fenceVals;
    IDXGISwapChain4* m_swapChain;
    Dx12Device& m_device;
    
};

}
}
