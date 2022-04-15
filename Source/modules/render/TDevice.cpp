#include "TDevice.h"
#include <Config.h>
#include <coalpy.core/Assert.h>
#if ENABLE_DX12
#include <dx12/Dx12Device.h>
#endif
#if ENABLE_VULKAN
#include <vulkan/VkDevice.h>
#endif

namespace coalpy
{
namespace render
{

void IDevice::enumerate(DevicePlat platform, std::vector<DeviceInfo>& outputList)
{
#if ENABLE_DX12
    if (platform == DevicePlat::Dx12)
        Dx12Device::enumerate(outputList);
#endif

#if ENABLE_VULKAN
    if (platform == DevicePlat::Vulkan)
        VkDevice::enumerate(outputList);
#endif
}

IDevice * IDevice::create(const DeviceConfig& config)
{
    CPY_ERROR(config.shaderDb != nullptr);
    if (config.shaderDb == nullptr)
        return nullptr;

#if ENABLE_DX12
    if (config.platform == DevicePlat::Dx12)
        return new Dx12Device(config);
#endif

#if ENABLE_VULKAN
    if (config.platform == DevicePlat::Vulkan)
        return new VkDevice(config);
#endif

    return nullptr;
}

}
}
