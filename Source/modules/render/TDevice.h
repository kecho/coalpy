#pragma once

#include <coalpy.render/IDevice.h>

namespace coalpy
{

class IShaderDb;

namespace render
{

template<class PlatDevice>
class TDevice : public IDevice
{
public:
    TDevice(const DeviceConfig& config);
    virtual ~TDevice();

    virtual Texture createTexture(const TextureDesc& desc) override;
    virtual const DeviceConfig& config() const override { return m_config; }

protected:
    IShaderDb& m_db;
    DeviceConfig m_config;
};

template<class PlatDevice>
TDevice<PlatDevice>::TDevice(const DeviceConfig& config)
: m_config(config), m_db(*config.shaderDb)
{
}

template<class PlatDevice>
TDevice<PlatDevice>::~TDevice()
{
}

template<class PlatDevice>
Texture TDevice<PlatDevice>::createTexture(const TextureDesc& desc)
{
    return ((PlatDevice*)this)->platCreateTexture(desc);
}

}

}
