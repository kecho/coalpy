#pragma once

#include <coalpy.render/IShaderDb.h>
#include <vector>
#include <string>
#include <functional>

struct IDxcBlob;
struct IDxcBlobUtf16;

namespace coalpy
{

class ByteBuffer;

using Dx12CompilerOnError = std::function<void(const char* name, const char* errorString)>;
using Dx12CompilerOnFinished = std::function<void(bool success, IDxcBlob* resultBlob, IDxcBlob* pdbBlob, IDxcBlobUtf16* pdbName)>;
using Dx12CompilerOnInclude = std::function<bool(const char* path, ByteBuffer& buffer)>;

struct Dx12CompileArgs
{
    ShaderType type;
    const char* shaderName;
    const char* mainFn;
    const char* source;
    const char* debugName;
    int sourceSize;
    std::vector<std::string> additionalIncludes;
    std::vector<std::string> defines;
    Dx12CompilerOnError onError;
    Dx12CompilerOnInclude onInclude;
    Dx12CompilerOnFinished onFinished;
    bool generatePdb;
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
