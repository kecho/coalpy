#include <coalpy.render/CommandList.h>
#include <coalpy.render/AbiCommands.h>
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{ 

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
    
    MemOffset offset = allocate<AbiComputeCmd>();
    return ComputeCommand(offset, &m_buffer);
}


template<typename AbiType>
MemOffset CommandList::allocate()
{
    auto t = (int)AbiType::sType;
    m_buffer.append(&t);
    auto offset = (MemOffset)m_buffer.size();
    m_buffer.appendEmpty(sizeof(AbiType));
    auto* abiObj = (AbiType*)(m_buffer.data() + offset);
    new (abiObj) AbiType;
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

    MemOffset offset = allocate<AbiUploadCmd>();
    return UploadCommand(offset, &m_buffer);
}

DownloadCommand CommandList::addDownloadCommand()
{
    CPY_ASSERT_MSG(!m_closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_closed)
        return {};

    MemOffset offset = allocate<AbiDownloadCmd>();
    return DownloadCommand(offset, &m_buffer);
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
    auto* cmdData = data<AbiDownloadCmd>();
    cmdData->source = source;
    cmdData->destinationOffset = (MemOffset)destinationBuffer;
    cmdData->destinationSize = destinationSize;
}

}
}
