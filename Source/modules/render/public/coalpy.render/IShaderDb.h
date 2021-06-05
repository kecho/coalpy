#pragma once

#include <coalpy.render/ShaderDefs.h>

namespace coalpy
{

class IShaderDb
{
public:
    virtual ShaderHandle requestCompile(const ShaderDesc& desc) = 0;
    virtual ShaderHandle requestCompile(const ShaderInlineDesc& desc) = 0;
    virtual void resolve(ShaderHandle handle) = 0;
    virtual bool isValid(ShaderHandle handle) const = 0;

    virtual ~IShaderDb(){}
    static IShaderDb* create(const ShaderDbDesc& desc);
};

}
