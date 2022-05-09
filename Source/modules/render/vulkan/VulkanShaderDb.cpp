#include <Config.h>
#include "VulkanShaderDb.h" 
#include "SpirvReflectionData.h"
#include <iostream>

#define DEBUG_PRINT_SPIRV_REFLECTION 0

#if ENABLE_VULKAN

namespace coalpy
{

namespace
{

const char* debugDescTypeName(SpvReflectDescriptorType type)
{
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return "SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
    default:
        return "unknown";
    }
}

}

VulkanShaderDb::VulkanShaderDb(const ShaderDbDesc& desc)
: BaseShaderDb(desc)
{
}

void VulkanShaderDb::onCreateComputePayload(const ShaderHandle& handle, ShaderState& shaderState)
{
    if (shaderState.spirVReflectionData == nullptr)
    {
        if (m_desc.onErrorFn != nullptr)
            m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "No SPIR-V reflection data found.");
        return;
    }

    if (!shaderState.shaderBlob)
        return;

    ShaderGPUPayload oldPayload = shaderState.payload;
    auto* oldSpirvPayload = (SpirvPayload*)oldPayload;
    if (oldSpirvPayload != nullptr)
    {
        std::cout << "TODO: implementt deferred delete for SpirvShaderPayload" << std::endl;
        delete oldSpirvPayload;
    }

    auto* payload = new SpirvPayload;
    shaderState.payload = payload;

#if DEBUG_PRINT_SPIRV_REFLECTION
    if (shaderState.spirVReflectionData)
    {
        std::cout << "Vulkan Reflection:" << shaderState.debugName << std::endl;
        for (int i = 0; i < (int)shaderState.spirVReflectionData->descriptorSets.size(); ++i)
        {
            std::cout << "\tDescriptor set - " << i << std::endl;
            SpvReflectDescriptorSet* setData = shaderState.spirVReflectionData->descriptorSets[i];
            for (int b = 0; b < (int)setData->binding_count; ++b)
            {
                SpvReflectDescriptorBinding* binding = setData->bindings[b];
                std::cout << "\tb[" << binding->binding << "] : " << debugDescTypeName(binding->descriptor_type);
                std::cout << " count[" << binding->count << "]" <<  std::endl;
            }
        }
    }
#endif
}

void VulkanShaderDb::onDestroyPayload(ShaderState& shaderState)
{
    ShaderGPUPayload payload = shaderState.payload;
    shaderState.payload = nullptr;

    if (!payload)
        return;

    auto* spirvPayload = (SpirvPayload*)payload;
    delete spirvPayload;
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
