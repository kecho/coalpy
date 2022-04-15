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


namespace coalpy
{

class VkShaderDb : public BaseShaderDb
{
public:
    explicit VkShaderDb(const ShaderDbDesc& desc);
    virtual ~VkShaderDb();

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
    void onDestroyPayload(ShaderState& state);
    bool updateComputePipelineState(ShaderState& state);
};

}
