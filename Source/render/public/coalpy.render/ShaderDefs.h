#pragma once

#include <coalpy.core/GenericHandle.h>

namespace coalpy
{

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
    const char* debugName;
    const char* path;
};

struct ShaderInlineDesc
{
    ShaderType type;
    const char* debugName;
    const char* immCode;
};

using ShaderHandle = GenericHandle<unsigned>;
using GpuPipelineHandle = GenericHandle<unsigned>;

struct ShaderCompilationResult
{
    bool success;
    ShaderHandle handle;
};

struct ShaderDbDesc
{
    //optional if we want to specify a path for the compiler library
    const char* compilerDllPath = nullptr;
};

}
