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

struct ID3D12PipelineState;

namespace coalpy
{

namespace render
{
    class Dx12Device;
}

class Dx12ShaderDb : public BaseShaderDb
{
public:
    explicit Dx12ShaderDb(const ShaderDbDesc& desc);
    virtual ~Dx12ShaderDb();

    render::Dx12Device* dx12Device() const { return (render::Dx12Device*)m_parentDevice; }

    ID3D12PipelineState* unsafeGetCsPso(ShaderHandle handle)
    {
        std::shared_lock lock(m_shadersMutex);
        ShaderGPUPayload payload = m_shaders[handle]->payload;
        return (ID3D12PipelineState*)payload;
    }

private:
    virtual void onCreateComputePayload(const ShaderHandle& handle, ShaderState& state) override;
    void onDestroyPayload(ShaderState& state);
    bool updateComputePipelineState(ShaderState& state);
};

}
