#pragma once

#include <BaseShaderDb.h>

@protocol MTLLibrary;
@protocol MTLComputePipelineState;

namespace coalpy
{

struct MetalPayload
{
    id<MTLLibrary> mtlLibrary = {};
    id<MTLComputePipelineState> mtlPipelineState = {};
};

class MetalShaderDb : public BaseShaderDb
{
public:
    explicit MetalShaderDb(const ShaderDbDesc& desc);
    virtual ~MetalShaderDb();

    MetalPayload* getMetalPayload(ShaderHandle handle)
    {
        std::shared_lock lock(m_shadersMutex);
        ShaderGPUPayload payload = m_shaders[handle]->payload;
        return (MetalPayload*)payload;
    }

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
};

}
