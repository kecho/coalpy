#include <Config.h>
#include <coalpy.render/IShaderDb.h>

#if ENABLE_DX12
#include "dx12/Dx12ShaderDb.h"
#endif

#if ENABLE_VULKAN
#include "vulkan/VulkanShaderDb.h"
#endif

#if ENABLE_METAL
#include "metal/MetalShaderDb.h"
#endif

namespace coalpy
{

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
#if ENABLE_DX12
    if (desc.platform == render::DevicePlat::Dx12)
        return new Dx12ShaderDb(desc);
#endif

#if ENABLE_VULKAN
    if (desc.platform == render::DevicePlat::Vulkan)
        return new VulkanShaderDb(desc);
#endif

#if ENABLE_METAL
    if (desc.platform == render::DevicePlat::Metal)
        return new MetalShaderDb(desc);
#endif

    return nullptr;
}

}
