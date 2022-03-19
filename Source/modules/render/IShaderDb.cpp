#include <Config.h>
#include <coalpy.render/IShaderDb.h>

#if ENABLE_DX12
#include "dx12/Dx12ShaderDb.h"
#endif

namespace coalpy
{

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
#if ENABLE_DX12
    return new Dx12ShaderDb(desc);
#else
    return nullptr;
#endif
}

}
