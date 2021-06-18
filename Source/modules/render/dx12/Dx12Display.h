#pragma once
#include <coalpy.render/Resources.h>
#include <coalpy.render/IDisplay.h>

struct IDXGISwapChain4;

namespace coalpy
{
namespace render
{

class Dx12Device;

class Dx12Display : public IDisplay
{
public:
    Dx12Display(const DisplayConfig& config, Dx12Device& device);
    virtual ~Dx12Display();
    virtual Texture texture() override;
    virtual void resize(unsigned int width, unsigned int height) override;

private:
    IDXGISwapChain4* m_swapChain;
    Dx12Device& m_device;
    
};

}
}
