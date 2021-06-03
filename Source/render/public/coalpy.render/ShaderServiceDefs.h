#pragma once

#include <coalpy.render/ShaderDefs.h>
#include <functional>

namespace coalpy
{

class IFileSystem;
class ITaskSystem;
class IShaderDb;

struct ShaderServiceDesc
{
    IShaderDb* db;
    const char* watchDirectory; 
    int fileWatchPollingRate;
};

}
