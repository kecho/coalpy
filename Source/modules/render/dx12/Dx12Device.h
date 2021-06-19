#pragma once

#include <TDevice.h>
#include <coalpy.render/Resources.h>

struct IDXGIFactory2;
struct ID3D12Device2;
struct ID3D12Debug;

namespace coalpy
{
namespace render
{

class Dx12Queues;

class Dx12Device : public TDevice<Dx12Device>
{
public:
    static IDXGIFactory2* dxgiFactory();

    Dx12Device(const DeviceConfig& config);
    virtual ~Dx12Device();

    static void enumerate(std::vector<DeviceInfo>& outputList);
    Texture platCreateTexture(const TextureDesc& desc);

    ID3D12Device2& device() { return *m_device; }
    Dx12Queues& queues() { return *m_queues; }

    virtual const DeviceInfo& info() const override { return m_info; }
    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) override;

private:
    ID3D12Debug* m_debugLayer;
    ID3D12Device2* m_device;
    Dx12Queues* m_queues;
    DeviceInfo m_info;
};

}
}

