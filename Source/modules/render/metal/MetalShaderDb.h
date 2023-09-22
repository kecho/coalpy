#pragma once

#include <BaseShaderDb.h>

namespace coalpy
{

class MetalShaderDb : public BaseShaderDb
{
public:
    explicit MetalShaderDb(const ShaderDbDesc& desc);
    virtual ~MetalShaderDb();

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
};

}
