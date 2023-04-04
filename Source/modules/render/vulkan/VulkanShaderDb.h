#pragma once

#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.files/Utils.h>
#include <BaseShaderDb.h>
#include <DxcCompiler.h>
#include <shared_mutex>
#include <atomic>
#include <set>
#include <vulkan/vulkan.h>

namespace coalpy
{

struct VulkanDescriptorSetInfo
{
    uint32_t setIndex;
    VkDescriptorSetLayout layout;
};

struct SpirvPayload
{
    VkShaderModule shaderModule = {};
    VkPipeline pipeline = {};
    VkPipelineLayout pipelineLayout = {};
    std::vector<VulkanDescriptorSetInfo> descriptorSetsInfos;
    uint64_t activeDescriptors[SpirvMaxRegisterSpace][(int)SpirvRegisterType::Count] = {};
    uint8_t  activeCounterRegister[SpirvRegisterTypeShiftCount] = {};
    uint64_t activeCountersBitMask[SpirvMaxRegisterSpace] = {};
    static void nextDescriptorRange(uint64_t& mask, int& beingIndex, int& count);
};

class VulkanShaderDb : public BaseShaderDb
{
public:
    explicit VulkanShaderDb(const ShaderDbDesc& desc);
    virtual ~VulkanShaderDb();
    const SpirvPayload& unsafeGetSpirvPayload(ShaderHandle handle) const
    {
        std::shared_lock lock(m_shadersMutex);
        ShaderGPUPayload payload = m_shaders[handle]->payload;
        return *((SpirvPayload*)payload);
    }

    void purgePayloads();

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
    void onDestroyPayload(ShaderState& state);
    bool updateComputePipelineState(ShaderState& state);
};

}
