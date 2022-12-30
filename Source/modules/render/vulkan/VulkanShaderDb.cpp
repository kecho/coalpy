#include <Config.h>
#include "VulkanShaderDb.h" 
#include "SpirvReflectionData.h"
#include "VulkanDevice.h"
#include <dxcapi.h>
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

    if (m_parentDevice == nullptr)
        return;

    render::VulkanDevice& vulkanDevice = *static_cast<render::VulkanDevice*>(m_parentDevice);
    
    // Create descriptor layouts
    auto* payload = new SpirvPayload;
    shaderState.payload = payload;
    std::vector<VkDescriptorSetLayout> layouts;
    {
        std::vector<VulkanDescriptorSetInfo> descriptorSetsInfos;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(64);
        auto stageFlags = (VkShaderStageFlags)shaderState.spirVReflectionData->module.shader_stage;
        for (int i = 0; i < (int)shaderState.spirVReflectionData->descriptorSets.size(); ++i)
        {
            bindings.clear();
            SpvReflectDescriptorSet* setData = shaderState.spirVReflectionData->descriptorSets[i];
            if (setData->set >= (uint32_t)SpirvMaxRegisterSpace)
            {
                std::cerr << "Error: found descriptor set " << setData->set
                          << ". Max binding for set is " << SpirvMaxRegisterSpace
                          << ". Set won't be bound, and it wil be ignored." << std::endl;
                continue;
            }
            uint64_t* descriptorBitsSections = payload->activeDescriptors[setData->set];
            for (int b = 0; b < (int)setData->binding_count; ++b)
            {
                SpvReflectDescriptorBinding& reflectionBinding = *setData->bindings[b];
                if (reflectionBinding.binding >= ((uint32_t)SpirvRegisterType::Count * (uint32_t)SpirvRegisterTypeShiftCount))
                {
                    std::cerr << "Spir binding out of range. We can only utilize up to " << SpirvRegisterTypeShiftCount 
                              << " per register type. Binding wil be ignored." << std::endl;
                    continue;
                }
                bindings.emplace_back();
                VkDescriptorSetLayoutBinding& binding = bindings.back();
                binding = {};
                binding.binding = reflectionBinding.binding;
                binding.descriptorType = (VkDescriptorType)reflectionBinding.descriptor_type;
                binding.descriptorCount = reflectionBinding.count;
                binding.stageFlags = stageFlags;
                uint64_t& activeDescriptorBits = descriptorBitsSections[(int)reflectionBinding.binding / (int)SpirvRegisterTypeShiftCount];
                activeDescriptorBits |= 1 << ((int)reflectionBinding.binding % (int)SpirvRegisterTypeShiftCount);
            }

            VkDescriptorSetLayout layout = {};
            VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
            layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
            layoutCreateInfo.pBindings = bindings.data();
            VK_OK(vkCreateDescriptorSetLayout(vulkanDevice.vkDevice(), &layoutCreateInfo, nullptr, &layout));
            descriptorSetsInfos.push_back(VulkanDescriptorSetInfo { setData->set, layout });
            layouts.push_back(layout);
        }
        payload->descriptorSetsInfos = std::move(descriptorSetsInfos);
    }

    // Create layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (int)layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    VK_OK(vkCreatePipelineLayout(vulkanDevice.vkDevice(), &pipelineLayoutInfo, nullptr, &payload->pipelineLayout));

    // Shader module
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = (uint32_t)shaderState.shaderBlob->GetBufferSize();
    shaderModuleInfo.pCode = (const uint32_t*)shaderState.shaderBlob->GetBufferPointer();
    VK_OK(vkCreateShaderModule(vulkanDevice.vkDevice(), &shaderModuleInfo, nullptr, &payload->shaderModule));

    // Compute pipeline
    VkPipelineCache cache = {};
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = payload->pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.pName = shaderState.spirVReflectionData->mainFn.c_str();
    pipelineInfo.stage.module = payload->shaderModule;
    VK_OK(vkCreateComputePipelines(vulkanDevice.vkDevice(), cache, 1, &pipelineInfo, nullptr, &payload->pipeline));

#if DEBUG_PRINT_SPIRV_REFLECTION
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
                std::cout << " count[" << binding->count << "]";
                std::cout << " spirv_id[" << binding->spirv_id << "]";
                std::cout << " uav_counter_id[" << binding->uav_counter_id << "]";
                if (binding->uav_counter_binding != nullptr)
                    std::cout << " counter_binding[" << binding->uav_counter_binding->binding << "]";
                std::cout << " " << binding->name;
                std::cout << std::endl;
            }
        }
    }
#endif

    shaderState.spirVReflectionData->Release();
    shaderState.spirVReflectionData = nullptr;
}

void VulkanShaderDb::onDestroyPayload(ShaderState& shaderState)
{
    ShaderGPUPayload payload = shaderState.payload;
    shaderState.payload = nullptr;

    if (!payload)
        return;

    auto& spirvPayload = *(SpirvPayload*)payload;

    if (m_parentDevice)
    {
        render::VulkanDevice& vulkanDevice = *static_cast<render::VulkanDevice*>(m_parentDevice);

        if (spirvPayload.shaderModule)
            vkDestroyShaderModule(vulkanDevice.vkDevice(), spirvPayload.shaderModule, nullptr);

        if (spirvPayload.pipeline)
            vkDestroyPipeline(vulkanDevice.vkDevice(), spirvPayload.pipeline, nullptr);

        if (spirvPayload.pipelineLayout)
            vkDestroyPipelineLayout(vulkanDevice.vkDevice(), spirvPayload.pipelineLayout, nullptr);

        for (auto& setInfo : spirvPayload.descriptorSetsInfos)
            vkDestroyDescriptorSetLayout(vulkanDevice.vkDevice(), setInfo.layout, nullptr);
    }

    delete &spirvPayload;
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
