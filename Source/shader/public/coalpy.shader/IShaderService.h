#pragma once

#include <coalpy.shader/ShaderDefs.h>

namespace coalpy
{

class IShaderService 
{
public:
    static IShaderService* create(const ShaderServiceDesc& desc);
    virtual void start() = 0;
    virtual void stop()  = 0;
    virtual ShaderHandle compileShader(const ShaderDesc& desc) = 0;
    virtual ShaderHandle compileInlineShader(const ShaderInlineDesc& desc) = 0;
    virtual GpuPipelineHandle createComputePipeline(const ComputePipelineDesc& desc) = 0;
};


}
