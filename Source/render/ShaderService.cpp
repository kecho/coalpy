#include "ShaderService.h" 
#include <coalpy.files/Utils.h>
#include <coalpy.core/Assert.h>
#include <coalpy.core/String.h>
#include <coalpy.render/IShaderDb.h>
#include <iostream>
#include <string>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define WATCH_SERVICE_DEBUG_OUTPUT 0

namespace coalpy
{

ShaderService::ShaderService(const ShaderServiceDesc& desc)
: m_db(desc.db)
{
}

void ShaderService::start()
{
}

void ShaderService::stop()
{
}

IShaderService* IShaderService::create(const ShaderServiceDesc& desc)
{
    return new ShaderService(desc);
}

}
