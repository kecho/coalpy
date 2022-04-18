#include <Config.h>
#include "VulkanShaderDb.h" 

#if ENABLE_VULKAN

namespace coalpy
{

VulkanShaderDb::VulkanShaderDb(const ShaderDbDesc& desc)
: BaseShaderDb(desc)
{
}

void VulkanShaderDb::onCreateComputePayload(const ShaderHandle& handle, ShaderState& shaderState)
{
    //TODO: create vulkan payload.
}

void VulkanShaderDb::onDestroyPayload(ShaderState& state)
{
    //TODO: destroy vulkan payload
}

VulkanShaderDb::~VulkanShaderDb()
{
    m_shaders.forEach([this](ShaderHandle handle, ShaderState* state)
    {
        onDestroyPayload(*state);
    });
}

}

#endif
