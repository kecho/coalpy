#include "Dx12ShaderDb.h" 

namespace coalpy
{

void Dx12ShaderDb::compileShader(const ShaderDesc& desc, ShaderCompilationResult& result)
{
}

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
    return new Dx12ShaderDb;
}

}
