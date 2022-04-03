#include <Config.h>

#if ENABLE_DX12

#include <string>
#include <sstream>
#include <coalpy.core/Assert.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.files/Utils.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/String.h>

#include "Dx12ShaderDb.h" 
#include "Dx12Device.h"
#include <coalpy.core/ByteBuffer.h>

#include <iostream>
#include <windows.h>
#include <d3d12.h>
#include <dxcapi.h>

namespace coalpy
{

Dx12ShaderDb::Dx12ShaderDb(const ShaderDbDesc& desc)
: BaseShaderDb(desc)
{
}

void Dx12ShaderDb::onCreateComputePayload(const ShaderHandle& handle, ShaderState& shaderState)
{
    bool psoResult = updateComputePipelineState(shaderState);
    if (!psoResult && m_desc.onErrorFn != nullptr)
        m_desc.onErrorFn(handle, shaderState.debugName.c_str(), "Compute PSO creation failed. Check D3D12 error log.");
}

void Dx12ShaderDb::onDestroyPayload(ShaderState& state)
{
    ShaderGPUPayload payload = state.payload;
    auto* csPso = (ID3D12PipelineState*)payload;
    if (csPso == nullptr)
        return;

    if (dx12Device())
        dx12Device()->deferRelease(*csPso);
    csPso->Release();
}

bool Dx12ShaderDb::updateComputePipelineState(ShaderState& state)
{
    CPY_ASSERT(dx12Device() != nullptr);
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = &dx12Device()->defaultComputeRootSignature();
    if (!state.shaderBlob)
        return false;

    desc.CS.pShaderBytecode = state.shaderBlob->GetBufferPointer();
    desc.CS.BytecodeLength = state.shaderBlob->GetBufferSize();
    ID3D12PipelineState* pso;
    HRESULT result = dx12Device()->device().CreateComputePipelineState(&desc, DX_RET(pso));
    ShaderGPUPayload payload = state.payload;
    auto* oldPso = (ID3D12PipelineState*)payload;
    state.payload = pso;
    if (oldPso)
    {
        if (dx12Device())
            dx12Device()->deferRelease(*oldPso);
        oldPso->Release();
    }
    return result == S_OK;
}

Dx12ShaderDb::~Dx12ShaderDb()
{
    m_shaders.forEach([this](ShaderHandle handle, ShaderState* state)
    {
        onDestroyPayload(*state);
    });
}

}

#endif
