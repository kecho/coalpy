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
    if (shaderState.mslData == nullptr)
    {
        if (m_desc.onErrorFn != nullptr)
            m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "No Metal Shading Language data found.");
        return;
    }
}

}

#endif // ENABLE_METAL