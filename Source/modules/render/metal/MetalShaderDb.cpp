#include <Config.h>
#if ENABLE_METAL

#include "MetalShaderDb.h" 

namespace coalpy
{

MetalShaderDb::MetalShaderDb(const ShaderDbDesc& desc)
: BaseShaderDb(desc)
{
}

MetalShaderDb::~MetalShaderDb()
{
}

void MetalShaderDb::onCreateComputePayload(const ShaderHandle& handle, ShaderState& shaderState)
{
    // TODO: Do actual stuff in here
}

}

#endif // ENABLE_METAL