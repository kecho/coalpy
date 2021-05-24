#pragma once

#include <coalpy.core/GenericHandle.h>
#include <functional>

namespace coalpy
{

class IFileSystem;
class ITaskSystem;

enum class ShaderType
{
    Compute,
    Count
};

struct ShaderDesc
{
    ShaderType type;
    const char* debugName;
    const char* path;
};

struct ShaderInlineDesc
{
    ShaderType type;
    const char* debugName;
    const char* immCode;
};

using ShaderHandle = GenericHandle<unsigned>;
using GpuPipelineHandle = GenericHandle<unsigned>;

struct ComputePipelineDesc
{
    ShaderHandle computeShader;
};

struct ShaderCompileResponse
{
    ShaderHandle handle;
};

struct GpuPipelineResponse
{
    GpuPipelineHandle handle;
};

using OnShaderCompleteFn = std::function<void(const ShaderCompileResponse& response)>;
using OnGpuPipelineCompleteFn = std::function<void(const GpuPipelineResponse& response)>;

struct ShaderServiceDesc
{
    IFileSystem* fs;
    ITaskSystem* ts;
    const char* watchDirectory; //must be absolute.
    OnShaderCompleteFn shaderCompiledFn;
    OnGpuPipelineCompleteFn gpuPipelineCompiledFn;
};

}
