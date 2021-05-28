#pragma once

#include <coalpy.render/IShaderDb.h>
#include <vector>
#include <string>
#include <functional>

struct IDxcBlob;

namespace coalpy
{

using OnError = std::function<void(const char* name, const char* errorString)>;
using OnFinished = std::function<void(bool success, IDxcBlob* resultBlob)>;

struct Dx12CompileArgs
{
    ShaderType type;
    const char* shaderName;
    const char* mainFn;
    const char* source;
    std::vector<std::string> defines;
    OnError onError;
    OnFinished onFinished;
};

class Dx12Compiler
{
public:
    Dx12Compiler(const ShaderDbDesc& desc);
    ~Dx12Compiler();

    void compileShader(const Dx12CompileArgs& args);

private:
    void setupDxc();
    ShaderDbDesc m_desc;
};

}
