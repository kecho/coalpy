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

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
};

}
