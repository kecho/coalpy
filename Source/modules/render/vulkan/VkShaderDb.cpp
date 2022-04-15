#include <Config.h>
#include "VkShaderDb.h" 

#if ENABLE_VULKAN

namespace coalpy
{

VkShaderDb::VkShaderDb(const ShaderDbDesc& desc)
: BaseShaderDb(desc)
{
}

void VkShaderDb::onCreateComputePayload(const ShaderHandle& handle, ShaderState& shaderState)
{
    //TODO: create vulkan payload.
}

void VkShaderDb::onDestroyPayload(ShaderState& state)
{
    //TODO: destroy vulkan payload
}

VkShaderDb::~VkShaderDb()
{
    m_shaders.forEach([this](ShaderHandle handle, ShaderState* state)
    {
        onDestroyPayload(*state);
    });
}

}

#endif
