#pragma once

#include <coalpy.render/IShaderDb.h>
#include <vector>
#include <string>

namespace coalpy
{

class Dx12Compiler
{
public:
    Dx12Compiler(const ShaderDbDesc& desc);
    ~Dx12Compiler();

    void compileShader(
        ShaderType type,
        const char* mainFn,
        const char* source,
        const std::vector<std::string>& defines,
        ShaderCompilationResult& result);

private:
    void setupDxc();
    ShaderDbDesc m_desc;
};

}
