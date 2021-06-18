#pragma once

#include <TDevice.h>
#include <coalpy.render/Resources.h>

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
};

}
}

