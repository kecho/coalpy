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

struct SpirvPayload
{
    VkShaderModule shaderModule = {};
    VkPipeline pipeline = {};
    VkPipelineLayout pipelineLayout = {};
    std::vector<VkDescriptorSetLayout> layouts;
    uint64_t activeDescriptors[SpirvMaxRegisterSpace][(int)SpirvRegisterType::Count] = {};
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

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
    void onDestroyPayload(ShaderState& state);
    bool updateComputePipelineState(ShaderState& state);
};

}
