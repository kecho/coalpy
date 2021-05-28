#include <Config.h>

#if ENABLE_DX12

#include <coalpy.core/Assert.h>

#include "Dx12ShaderDb.h" 
#include "Dx12Compiler.h"

namespace coalpy
{

Dx12ShaderDb::Dx12ShaderDb(const ShaderDbDesc& desc)
: m_compiler(*(new Dx12Compiler(desc)))
, m_desc(desc)
{
}

Dx12ShaderDb::~Dx12ShaderDb()
{
    delete &m_compiler;
}

void Dx12ShaderDb::compileShader(const ShaderDesc& desc, ShaderCompilationResult& result)
{
}

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
    return new Dx12ShaderDb(desc);
}

}

#endif
