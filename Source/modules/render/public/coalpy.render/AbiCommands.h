#pragma once

#include <coalpy.render/ShaderDefs.h>
#include <coalpy.render/Resources.h>

namespace coalpy
{
namespace render
{

typedef uint64_t MemOffset;
typedef uint64_t MemSize;

const int someIntVal = 'CMDL';

enum class AbiCmdTypes : unsigned int
{
    CommandListSentinel = 'CMDL',
    CommandListEndSentinel = 'CMDE',
    Compute = 'COMP', 
    Copy = 'COPY', 
    Upload = 'UPLD',
    Download = 'DWLD',
    ClearAppendConsumeCounter = 'CLAC',
    CopyAppendConsumeCounter = 'CYAC',
    BeginMarker = 'BMKR',
    EndMarker = 'EMKR',
};

template<typename ElementType>
struct AbiPtr
{
    MemOffset offset = (MemOffset)-1;
    const size_t stride() const { return sizeof(ElementType); }
    ElementType* data(unsigned char* buffer) { return (ElementType*)(buffer + offset); }
    const ElementType* data(const unsigned char* buffer) const { return (const ElementType*)(buffer + offset); }
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
    MemSize cmdSize = {};

    ShaderHandle shader;

    AbiPtr<Buffer> constants;
    int       constantCounts = 0;

    AbiPtr<char> inlineConstantBuffer;
    int       inlineConstantBufferSize = 0;

    AbiPtr<InResourceTable> inResourceTables;
    int       inResourceTablesCounts = 0;

    AbiPtr<OutResourceTable> outResourceTables;
    int       outResourceTablesCounts = 0;

    AbiPtr<SamplerTable> samplerTables;
    int       samplerTablesCounts = 0;

    Buffer indirectArguments;

    AbiPtr<char> debugName;
    int debugNameSize = 0;
    int isIndirect = 0;
    int x = 1;
    int y = 1;
    int z = 1;
};

struct AbiCopyCmd
{
    int sentinel = (int)AbiCmdTypes::Copy;
    MemSize cmdSize = {};
    ResourceHandle source;
    ResourceHandle destination;

    int fullCopy = 0;
    int srcMipLevel = 0;
    int dstMipLevel = 0;
    int sourceX = -1;
    int sourceY = -1;
    int sourceZ = -1;

    int destX = 0;
    int destY = 0;
    int destZ = 0;

    int sizeX = 0;
    int sizeY = 0;
    int sizeZ = 0;
};

struct AbiUploadCmd
{
    int sentinel = (int)AbiCmdTypes::Upload;
    MemSize cmdSize = {};
    ResourceHandle destination;
    AbiPtr<char> sources; 
    int sourceSize = 0; 
    int mipLevel = 0;
    int sizeX = -1;
    int sizeY = -1;
    int sizeZ = -1;
    int destX = 0;
    int destY = 0;
    int destZ = 0;
};

struct AbiDownloadCmd
{
    int sentinel = (int)AbiCmdTypes::Download;
    MemSize cmdSize = {};
    ResourceHandle source;
    int mipLevel = 0;
    int arraySlice = 0;
};

struct AbiClearAppendConsumeCounter
{
    int sentinel = (int)AbiCmdTypes::ClearAppendConsumeCounter;
    MemSize cmdSize = {};
    ResourceHandle source;
    int counter;
};

struct AbiCopyAppendConsumeCounter
{
    int sentinel = (int)AbiCmdTypes::CopyAppendConsumeCounter;
    MemSize cmdSize = {};
    ResourceHandle source;
    ResourceHandle destination;
    int destinationOffset = 0;
};

struct AbiBeginMarker
{
    int sentinel = (int)AbiCmdTypes::BeginMarker;
    MemSize cmdSize = {};
    AbiPtr<char> str;
};

struct AbiEndMarker
{
    int sentinel = (int)AbiCmdTypes::EndMarker;
    MemSize cmdSize = {};
};

}
}
