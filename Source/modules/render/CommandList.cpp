#include <coalpy.render/CommandList.h>
#include <coalpy.core/Assert.h>

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

CommandList::CommandList()
: m_closed(false)
{
}

CommandList::~CommandList()
{
}

ComputeCommand CommandList::addComputeCommand()
{
    CPY_ASSERT_MSG(!m_closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_closed)
        return {};
    
    return {};
}


template<typename AbiType>
MemOffset CommandList::allocate()
{
    auto t = (int)AbiType::sType;
    m_buffer.append(&t);
    auto offset = (MemOffset)m_buffer.size();
    m_buffer.appendEmpty(sizeof(AbiType));
    return offset;
}

CopyCommand CommandList::addCopyCommand()
{
    CPY_ASSERT_MSG(!m_closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_closed)
        return {};

    MemOffset offset = allocate<AbiCopyCmd>();
    return CopyCommand(offset, &m_buffer);
}

UploadCommand CommandList::addUploadCommand()
{
    CPY_ASSERT_MSG(!m_closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_closed)
        return {};

    return {};
}


void ComputeCommand::setShader(ShaderHandle shader)
{
}

void ComputeCommand::setConstants(Buffer* constBuffers, int bufferCounts)
{
}

void ComputeCommand::setInResources(InResourceTable* inTables, int inputTablesCount)
{
}

void ComputeCommand::setOutResources(OutResourceTable* outTables, int inputTablesCount)
{
}

void ComputeCommand::setDispatch(const char* debugNameMarker, int x, int y, int z)
{
}

void CopyCommand::setResources(ResourceHandle source, ResourceHandle destination)
{
}

void UploadCommand::setData(const void* source, int sourceSize, ResourceHandle destination)
{
}

void DownloadCommand::setData(ResourceHandle source, void* destinationBuffer, int destinationSize)
{
}

}
}
