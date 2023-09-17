#include <Config.h>
#if ENABLE_VULKAN

#include "VulkanShaderDb.h" 
#include "SpirvReflectionData.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "VulkanGc.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <dxcapi.h>
#include <iostream>
#include <string>
#include <unordered_map>

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


static const char* processCounterByName(
    SpvReflectDescriptorBinding& reflectionBinding)
{
    if (reflectionBinding.resource_type != SPV_REFLECT_RESOURCE_FLAG_UAV)
        return nullptr;

    if (reflectionBinding.descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        return nullptr;

    static const int counterValLen = 12; //12 strlen("counter.var.")
    const char* result = strstr(reflectionBinding.name, "counter.var.");
    if (!result)
        return nullptr;

    return result + counterValLen;
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

    render::VulkanDevice& vulkanDevice = *static_cast<render::VulkanDevice*>(m_parentDevice);
    ShaderGPUPayload oldPayload = shaderState.payload;
    auto* oldSpirvPayload = (SpirvPayload*)oldPayload;
    if (oldSpirvPayload != nullptr)
    {
        vulkanDevice.gc().deferRelease(
            oldSpirvPayload->pipelineLayout,
            oldSpirvPayload->pipeline,
            oldSpirvPayload->shaderModule);
        delete oldSpirvPayload;
    }

    if (m_parentDevice == nullptr)
        return;
    
    // Create descriptor layouts
    auto* payload = new SpirvPayload;
    shaderState.payload = payload;
    std::unordered_map<std::string, SpvReflectDescriptorBinding*> bindingToCounterMap;
    std::vector<VkDescriptorSetLayout> layouts;
    {
        std::vector<VulkanDescriptorSetInfo> descriptorSetsInfos;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorBindingFlags> bindingsFlags;
        bindings.reserve(64);
        auto stageFlags = (VkShaderStageFlags)shaderState.spirVReflectionData->module.shader_stage;
        for (int i = 0; i < (int)shaderState.spirVReflectionData->descriptorSets.size(); ++i)
        {
            bindings.clear();
            bindingsFlags.clear();
            SpvReflectDescriptorSet* setData = shaderState.spirVReflectionData->descriptorSets[i];
            if (setData->set >= (uint32_t)SpirvMaxRegisterSpace)
            {
                std::cerr << "Error: found descriptor set " << setData->set
                          << ". Max binding for set is " << SpirvMaxRegisterSpace
                          << ". Set won't be bound, and it wil be ignored." << std::endl;
                continue;
            }
            std::unordered_map<std::string, SpvReflectDescriptorBinding*> bindingCounter;
            uint64_t* descriptorBitsSections = payload->activeDescriptors[setData->set];
            uint64_t& activeCountersBitMask = payload->activeCountersBitMask[setData->set];
            for (int b = 0; b < (int)setData->binding_count; ++b)
            {
                SpvReflectDescriptorBinding& reflectionBinding = *setData->bindings[b];
                if (reflectionBinding.binding >= ((uint32_t)SpirvRegisterType::Count * (uint32_t)SpirvRegisterTypeShiftCount))
                {
                    std::cerr << "Spirv binding out of range. We can only utilize up to " << SpirvRegisterTypeShiftCount 
                              << " per register type. Binding wil be ignored." << std::endl;
                    continue;
                }
                
                SpirvRegisterType implicitRegisterType = (SpirvRegisterType)((int)reflectionBinding.binding / (int)SpirvRegisterTypeShiftCount);
                if (implicitRegisterType < SpirvRegisterType::b || implicitRegisterType >= SpirvRegisterType::Count)
                {
                    std::cerr << " Coalpy could not implicitely figure out the register type to map to hlsl for descriptor bining " << reflectionBinding.binding
                              << " on descriptor set " << setData->set << std::endl;
                    continue;
                }

                const char* counterOwner = processCounterByName(reflectionBinding);
                if (counterOwner != nullptr && implicitRegisterType != SpirvRegisterType::b)
                {
                    std::cerr << " Coalpy stuffs append consume counters in the first 32 bindings of descriptor set 0 (reserved for constant buffers)."
                              << " Binding with name " << reflectionBinding.name << " has been parsed as a counter. "
                              << " This binding lives out of the range of register space0. This means you have too many constant buffer resources" << std::endl;
                    continue;
                }

                if (counterOwner != nullptr)
                {
                    bindingCounter[std::string(counterOwner)] = &reflectionBinding;
                    activeCountersBitMask |= 1 << reflectionBinding.binding;
                }
                else if (!bindingCounter.empty())
                {
                    auto bindingCounterIt = bindingCounter.find(std::string(reflectionBinding.name));
                    if (bindingCounterIt != bindingCounter.end())
                    {
                        const int uavOffset = SpirvRegisterTypeOffset(SpirvRegisterType::u);
                        if (reflectionBinding.binding < uavOffset || reflectionBinding.binding >= (uavOffset + SpirvRegisterTypeShiftCount))
                            std::cerr << "Failed to connect counter of binding " << reflectionBinding.name << " with binding slot " << reflectionBinding.binding << std::endl;
                        else
                            payload->activeCounterRegister[reflectionBinding.binding - uavOffset] = bindingCounterIt->second->binding;
                    }
                }
                
                bindings.emplace_back();
                bindingsFlags.emplace_back();
                VkDescriptorSetLayoutBinding& binding = bindings.back();
                VkDescriptorBindingFlags& flags = bindingsFlags.back();
                binding = {};
                flags = {};
                binding.binding = reflectionBinding.binding;
                binding.descriptorType = (VkDescriptorType)reflectionBinding.descriptor_type;
                binding.descriptorCount = reflectionBinding.count;
                binding.stageFlags = stageFlags;
                uint64_t& activeDescriptorBits = descriptorBitsSections[(int)implicitRegisterType];
                activeDescriptorBits |= 1 << ((int)reflectionBinding.binding % (int)SpirvRegisterTypeShiftCount); //TODO: bindless
                //if (reflectionBinding.count)
                //{
                //    flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                //    flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                //}
            }

            VkDescriptorSetLayout layout = {};
            VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr };
            layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
            layoutCreateInfo.pBindings = bindings.data();

            VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlagsCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, nullptr };
            if ((vulkanDevice.enabledDeviceExts() & render::asFlag(render::VulkanDeviceExtensions::DescriptorIndexing)) != 0)
            {
                layoutFlagsCreateInfo.bindingCount = (uint32_t)bindingsFlags.size();
                layoutFlagsCreateInfo.pBindingFlags = bindingsFlags.data();
                layoutFlagsCreateInfo.pNext = layoutCreateInfo.pNext;
                layoutCreateInfo.pNext = &layoutFlagsCreateInfo;
            }

            VK_OK(vkCreateDescriptorSetLayout(vulkanDevice.vkDevice(), &layoutCreateInfo, nullptr, &layout));
            if (setData->set >= layouts.size())
            {
                descriptorSetsInfos.resize(setData->set + 1, {});
                layouts.resize(setData->set + 1, VK_NULL_HANDLE);
            }
            descriptorSetsInfos[setData->set] = VulkanDescriptorSetInfo { setData->set, layout };
            layouts[setData->set] = layout;
        }
        payload->descriptorSetsInfos = std::move(descriptorSetsInfos);
    }

    //Patch layouts that are null
    {
        for (uint32_t set = 0; set < (uint32_t)layouts.size(); ++set)
        {
            auto& layout = layouts[set];
            if (layout != VK_NULL_HANDLE)
                continue;
            
            VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr };
            VK_OK(vkCreateDescriptorSetLayout(vulkanDevice.vkDevice(), &layoutCreateInfo, nullptr, &layout));

            payload->descriptorSetsInfos[set] = VulkanDescriptorSetInfo { set, layout };
        }
    }
    
    // Create layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (int)layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    VK_OK(vkCreatePipelineLayout(vulkanDevice.vkDevice(), &pipelineLayoutInfo, nullptr, &payload->pipelineLayout));

    if (m_desc.spirvPrintReflectionInfo)
    {
        std::cout << "SPIRV Reflection:" << shaderState.debugName << "{" << std::endl;
        for (int i = 0; i < (int)shaderState.spirVReflectionData->descriptorSets.size(); ++i)
        {
            SpvReflectDescriptorSet* setData = shaderState.spirVReflectionData->descriptorSets[i];
            std::cout << "\tDescriptor set - " << setData->set << std::endl;
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
            std::cout << "}" << std::endl;
        }
    }

    // Shader module
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = (uint32_t)shaderState.shaderBlob->GetBufferSize();
    shaderModuleInfo.pCode = (const uint32_t*)shaderState.shaderBlob->GetBufferPointer();
    VK_OK(vkCreateShaderModule(vulkanDevice.vkDevice(), &shaderModuleInfo, nullptr, &payload->shaderModule));

    // Compute pipeline
    VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr };
    pipelineInfo.layout = payload->pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.pName = shaderState.spirVReflectionData->mainFn.c_str();
    pipelineInfo.stage.module = payload->shaderModule;
    VK_OK(vkCreateComputePipelines(vulkanDevice.vkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &payload->pipeline));

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
    purgePayloads();
}

void VulkanShaderDb::purgePayloads()
{
    m_shaders.forEach([this](ShaderHandle handle, ShaderState* state)
    {
        onDestroyPayload(*state);
    });
}

}

#endif

#endif // ENABLE_VULKAN