#pragma once

#include <coalpy.render/ShaderDefs.h>
#include <coalpy.render/Resources.h>

namespace coalpy
{
namespace render
{

typedef uint64_t MemOffset;
typedef uint64_t MemSize;

enum class AbiCmdTypes : int
{
    CommandListSentinel = 1,
    Compute = 2, 
    Copy = 3, 
    Upload = 4,
    Download = 5,
};

struct AbiComputeCmd
{
    static const AbiCmdTypes sType = AbiCmdTypes::Compute;

    ShaderHandle shader;

    MemOffset constants;
    int       constantCounts;

    MemOffset inResourceTables;
    int       inResourceCounts;

    MemOffset outResourceTables;
    int       outResourceCounts;

    MemOffset debugName;
    int debugNameSize;
    int x;
    int y;
    int z;
};

struct AbiCopyCmd
{
    static const AbiCmdTypes sType = AbiCmdTypes::Copy;
    ResourceHandle source;
    ResourceHandle destination;
};

struct AbiUploadCmd
{
    static const AbiCmdTypes sType = AbiCmdTypes::Upload;
    ResourceHandle destination;
    MemOffset sourceOffset; 
    MemSize   sourceSize; 
};

struct AbiDownloadCmd
{
    static const AbiCmdTypes sType = AbiCmdTypes::Download;
    ResourceHandle source;
    MemOffset destinationOffset; 
    MemSize   destinationSize; 
};

}
}
