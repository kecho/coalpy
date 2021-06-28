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
    CommandListSentinel = 'CMDL',
    Compute = 'COMP', 
    Copy = 'COPY', 
    Upload = 'UPLD',
    Download = 'DWLD'
};

template<typename ElementType>
struct AbiPtr
{
    MemOffset offset = (MemOffset)-1;
    const size_t stride() const { return sizeof(ElementType); }
    ElementType* data(unsigned char* buffer) { return (ElementType*)(buffer + offset); }
};

struct AbiCommandListHeader
{
    static const int sVersion = 1;
    int sentinel = (int)AbiCmdTypes::CommandListSentinel;
    int version = sVersion;

    MemSize commandListSize = 0ull;
};

struct AbiComputeCmd
{
    int sentinel = (int)AbiCmdTypes::Compute;

    ShaderHandle shader;

    AbiPtr<Buffer> constants;
    int       constantCounts = 0;

    AbiPtr<InResourceTable> inResourceTables;
    int       inResourceCounts = 0;

    AbiPtr<OutResourceTable> outResourceTables;
    int       outResourceCounts = 0;

    AbiPtr<char> debugName;
    int debugNameSize = 0;
    int x = 1;
    int y = 1;
    int z = 1;
};

struct AbiCopyCmd
{
    int sentinel = (int)AbiCmdTypes::Copy;
    ResourceHandle source;
    ResourceHandle destination;
};

struct AbiUploadCmd
{
    int sentinel = (int)AbiCmdTypes::Upload;
    ResourceHandle destination;
    AbiPtr<char> sources; 
    int  sourcesCount = 0; 
};

struct AbiDownloadCmd
{
    int sentinel = (int)AbiCmdTypes::Download;
    ResourceHandle source;
};

}
}
