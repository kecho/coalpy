#pragma once

#include <coalpy.shader/ShaderServiceDefs.h>

namespace coalpy
{

class IShaderService 
{
public:
    static IShaderService* create(const ShaderServiceDesc& desc);
    virtual void start() = 0;
    virtual void stop()  = 0;
};


}
