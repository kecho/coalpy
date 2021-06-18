#pragma once

#include <TDevice.h>
#include <coalpy.render/Resources.h>

struct ID3D12Device2;
struct ID3D12Debug;

namespace coalpy
{
namespace render
{

class Dx12Device : public TDevice<Dx12Device>
{
public:
    Dx12Device(const DeviceConfig& config);
    virtual ~Dx12Device();

    static void enumerate(std::vector<DeviceInfo>& outputList);
    Texture platCreateTexture(const TextureDesc& desc);

    ID3D12Device2& device() { return *m_device; }

    virtual const DeviceInfo& info() const override { return m_info; }

private:
    ID3D12Debug* m_debugLayer;
    ID3D12Device2* m_device;
    DeviceInfo m_info;
};

}
}

