#pragma once

#include <coalpy.core/GenericHandle.h>

namespace coalpy
{

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
};

struct ShaderInlineDesc
{
    ShaderType type;
    const char* name;
    const char* mainFn;
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
    IFileSystem* fs = nullptr;
    ITaskSystem* ts = nullptr;
};

}
