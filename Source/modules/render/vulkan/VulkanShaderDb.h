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
    VkDescriptorSetLayout layout;
};

class VulkanShaderDb : public BaseShaderDb
{
public:
    explicit VulkanShaderDb(const ShaderDbDesc& desc);
    virtual ~VulkanShaderDb();

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
    void onDestroyPayload(ShaderState& state);
    bool updateComputePipelineState(ShaderState& state);
};

}
