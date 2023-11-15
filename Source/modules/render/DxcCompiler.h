#pragma once

#include <coalpy.render/IShaderDb.h>
#include <vector>
#include <string>
#include <functional>

struct IDxcBlob;
struct IDxcBlobWide;

namespace coalpy
{

class ByteBuffer;
class SpirvReflectionData;

struct DxcResultPayload
{
    IDxcBlob* resultBlob;
    IDxcBlob* pdbBlob;
    IDxcBlobWide* pdbName;
    SpirvReflectionData* spirvReflectionData;
#if ENABLE_METAL
    std::string mslSource;
#endif // ENABLE_METAL
};

enum
{
    MaxDxcCompilerResourceTables = 8,
    SpirvRegisterTypeShiftCount = 32,
    SpirvMaxRegisterSpace = MaxDxcCompilerResourceTables
};

enum class SpirvRegisterType : int
{
    b, t, s, u, Count
};

inline int SpirvRegisterTypeOffset(SpirvRegisterType t)
{
    return (int)t * (int)SpirvRegisterTypeShiftCount;
}

inline wchar_t* SpirvRegisterTypeName(SpirvRegisterType t)
{
    switch(t)
    {
    case SpirvRegisterType::t:
        return L"t";
    case SpirvRegisterType::b:
        return L"b";
    case SpirvRegisterType::u:
        return L"u";
    case SpirvRegisterType::s:
        return L"s";
    default:
        return L"t";
    }
}

using DxcCompilerOnError = std::function<void(const char* name, const char* errorString)>;
using DxcCompilerOnFinished = std::function<void(bool success, DxcResultPayload& payload)>;
using DxcCompilerOnInclude = std::function<bool(const char* path, ByteBuffer& buffer)>;

struct DxcCompileArgs
{
    ShaderType type;
    ShaderModel shaderModel;
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
