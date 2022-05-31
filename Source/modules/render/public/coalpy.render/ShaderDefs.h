#pragma once

#include <coalpy.core/GenericHandle.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/ShaderModels.h>
#include <functional>
#include <vector>
#include <string>

namespace coalpy
{

class IFileWatcher;
class IFileSystem;
class ITaskSystem;

enum class ShaderType
{
    Vertex,
    Pixel,
    Compute,
    Count
};

struct ShaderDesc
{
    ShaderType type;
    const char* name;
    const char* mainFn;
    const char* path;
    std::vector<std::string> defines;
};

struct ShaderInlineDesc
{
    ShaderType type;
    const char* name;
    const char* mainFn;
    const char* immCode;
    std::vector<std::string> defines;
};

using ShaderHandle = GenericHandle<unsigned>;
using GpuPipelineHandle = GenericHandle<unsigned>;

struct ShaderCompilationResult
{
    bool success;
    ShaderHandle handle;
};

using OnShaderErrorFn = std::function<void(ShaderHandle handle, const char* shaderName, const char* shaderErrorStr)>;

struct ShaderDbDesc
{
    //optional if we want to specify a path for the compiler library
    render::DevicePlat platform = render::DevicePlat::Dx12;
    const char* compilerDllPath = nullptr;
    IFileSystem* fs = nullptr;
    ITaskSystem* ts = nullptr;
    IFileWatcher* fw = nullptr;
    OnShaderErrorFn onErrorFn = nullptr;
    bool resolveOnDestruction = false;
    bool enableLiveEditing = false;
    ShaderModel shaderModel = ShaderModel::Sm6_5;
    bool dumpPDBs = false;
};

}

namespace std
{
    template<>
    struct hash<coalpy::ShaderHandle>
    {
        std::size_t operator()(const coalpy::ShaderHandle& h) const
        {
            return (std::size_t)h.handleId;
        }
    };
}
