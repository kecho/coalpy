#pragma once

#include <coalpy.render/ShaderDefs.h>

namespace coalpy
{

class IShaderDb
{
public:
    virtual void compileShader(const ShaderDesc& desc, ShaderCompilationResult& result) = 0;
    virtual ~IShaderDb(){}
    static IShaderDb* create(const ShaderDbDesc& desc);
};

}
