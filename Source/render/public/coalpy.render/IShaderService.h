#pragma once

#include <coalpy.render/ShaderServiceDefs.h>

namespace coalpy
{

class IShaderService 
{
public:
    static IShaderService* create(const ShaderServiceDesc& desc);

    virtual ~IShaderService() {}
    virtual void start() = 0;
    virtual void stop()  = 0;
};


}
