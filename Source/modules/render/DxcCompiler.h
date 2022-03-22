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

using DxcCompilerOnError = std::function<void(const char* name, const char* errorString)>;
using DxcCompilerOnFinished = std::function<void(bool success, IDxcBlob* resultBlob, IDxcBlob* pdbBlob, IDxcBlobUtf16* pdbName)>;
using DxcCompilerOnInclude = std::function<bool(const char* path, ByteBuffer& buffer)>;

struct DxcCompileArgs
{
    ShaderType type;
    const char* shaderName;
    const char* mainFn;
    const char* source;
    const char* debugName;
    int sourceSize;
    std::vector<std::string> additionalIncludes;
    std::vector<std::string> defines;
    DxcCompilerOnError onError;
    DxcCompilerOnInclude onInclude;
    DxcCompilerOnFinished onFinished;
    bool generatePdb;
};

class DxcCompiler
{
public:
    DxcCompiler(const ShaderDbDesc& desc);
    ~DxcCompiler();

    void compileShader(const DxcCompileArgs& args);

private:
    void setupDxc();
    ShaderDbDesc m_desc;
};

}
