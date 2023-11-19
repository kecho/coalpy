#pragma once

#include <BaseShaderDb.h>

@protocol MTLLibrary;

namespace coalpy
{

struct MetalPayload
{
    id<MTLLibrary> mtlLibrary = {};
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
