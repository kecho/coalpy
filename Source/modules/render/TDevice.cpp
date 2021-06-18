#include "TDevice.h"
#include <dx12/Dx12Device.h>
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

void IDevice::enumerate(DevicePlat platform, std::vector<DeviceInfo>& outputList)
{
    if (platform == DevicePlat::Dx12)
        Dx12Device::enumerate(outputList);
}

IDevice * IDevice::create(const DeviceConfig& config)
{
    CPY_ERROR(config.shaderDb != nullptr);
    if (config.shaderDb == nullptr)
        return nullptr;

    if (config.platform == DevicePlat::Dx12)
        return new Dx12Device(config);

    return nullptr;
}

}
}
