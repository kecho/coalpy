#pragma once

#include <coalpy.core/Os.h>
#include <coalpy.render/Resources.h>
#include <string>
#include <vector>

namespace coalpy
{

class IShaderDb;

namespace render
{

enum class DevicePlat
{
    Dx12
};

struct DeviceConfig
{
    DevicePlat platform = DevicePlat::Dx12;
    ModuleOsHandle moduleHandle = nullptr;
    IShaderDb* shaderDb = nullptr;
    int index = -1;
};

struct DeviceInfo
{
    int valid;
    int index;
    std::string name;
};

class IDevice
{
public:
	virtual ~IDevice() {}
    static void enumerate(DevicePlat platform, std::vector<DeviceInfo>& outputList);
    static IDevice * create(const DeviceConfig& config);
    virtual const DeviceConfig& config() const = 0;

    virtual Texture createTexture(const TextureDesc& config) = 0;
};


}
}
