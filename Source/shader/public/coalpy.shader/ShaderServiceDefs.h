#pragma once

#include <coalpy.render/ShaderDefs.h>
#include <functional>

namespace coalpy
{

class IFileSystem;
class ITaskSystem;

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
    int fileWatchPollingRate;
    OnShaderCompleteFn shaderCompiledFn;
    OnGpuPipelineCompleteFn gpuPipelineCompiledFn;
};

}
