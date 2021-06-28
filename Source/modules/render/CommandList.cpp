#include <coalpy.render/CommandList.h>
#include <coalpy.render/AbiCommands.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/Assert.h>
#include <vector>

namespace coalpy
{
namespace render
{

struct CmdPendingMemory
{
    const u8* src = nullptr;
    const MemSize srcByteSize = 0;
    MemOffset destinationOffset; //offset to pointer / size blob member
};

class InternalCommandList
{
public:
    ByteBuffer buffer;
    std::vector<CmdPendingMemory> pendingMemory;
    bool closed = false;

    template<typename ElementType>
    int registerArrayParam(AbiPtr<ElementType>& ptr)
    {
        int index = (int)pendingMemory.size();
        pendingMemory.emplace_back();
        pendingMemory.back().destinationOffset = (u8*)&ptr.offset - buffer.data();
        return index;
    }

    template<typename ElementType>
    void storeArrayParam(int paramIndex, const ElementType* srcArray, int counts)
    {
        CPY_ASSERT(paramIndex >= 0 && paramIndex < (int)pendingMemory.srcArray());
        auto& pendingMem = pendingMemory[paramIndex];
        pendingMem.src         = (const u8*)srcArray;
        pendingMem.srcByteSize = (MemSize)(sizeof(ElementType) * counts);
    }
};

CommandList::CommandList()
: m_internal(*(new InternalCommandList()))
{
    allocate<AbiCommandListHeader>();
}

CommandList::~CommandList()
{
    delete &m_internal;
}

u8* CommandList::data()
{
    return m_internal.buffer.data();
}

size_t CommandList::size() const
{
    return m_internal.buffer.size();
}

void CommandList::close()
{
    if (m_internal.closed)
        return;

    for (auto& pendingMem : m_internal.pendingMemory)
    {
        MemOffset currOffset = (MemOffset)m_internal.buffer.size();
        u8* dataPtr = m_internal.buffer.data();
        CPY_ASSERT(pendingMem.destinationOffset <= (currOffset - sizeof(MemOffset)));
        *((MemOffset*)(dataPtr + pendingMem.destinationOffset)) = currOffset;
        m_internal.buffer.append(pendingMem.src, pendingMem.srcByteSize);
    }

    m_internal.pendingMemory = {};

    //store the size
    AbiCommandListHeader& header = *((AbiCommandListHeader*)(m_internal.buffer.data()));
    header.commandListSize = m_internal.buffer.size();

    m_internal.closed = true;
    
}

template<typename AbiType>
MemOffset CommandList::allocate()
{
    auto& buffer = m_internal.buffer;
    auto offset = (MemOffset)buffer.size();
    buffer.appendEmpty(sizeof(AbiType));
    auto* abiObj = (AbiType*)(buffer.data() + offset);
    new (abiObj) AbiType;
    return offset;
}

ComputeCommand CommandList::addComputeCommand()
{
    CPY_ASSERT_MSG(!m_internal.closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_internal.closed)
        return {};
    
    MemOffset offset = allocate<AbiComputeCmd>();
    return ComputeCommand(offset, &m_internal);
}

CopyCommand CommandList::addCopyCommand()
{
    CPY_ASSERT_MSG(!m_internal.closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_internal.closed)
        return {};

    MemOffset offset = allocate<AbiCopyCmd>();
    return CopyCommand(offset, &m_internal);
}

UploadCommand CommandList::addUploadCommand()
{
    CPY_ASSERT_MSG(!m_internal.closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_internal.closed)
        return {};

    MemOffset offset = allocate<AbiUploadCmd>();
    return UploadCommand(offset, &m_internal);
}

DownloadCommand CommandList::addDownloadCommand()
{
    CPY_ASSERT_MSG(!m_internal.closed, "Command list has been closed. Mutability is not permitted anymore.");
    if (m_internal.closed)
        return {};

    MemOffset offset = allocate<AbiDownloadCmd>();
    return DownloadCommand(offset, &m_internal);
}

ComputeCommand::ComputeCommand(MemOffset offset, InternalCommandList* parent)
: GpuCommand(offset, parent)
{
    auto& cmdData = *data<AbiComputeCmd>();
    m_inputResourcesParamIndex  = parent->registerArrayParam(cmdData.constants);
    m_outputResourcesParamIndex = parent->registerArrayParam(cmdData.inResourceTables);
    m_constantBuffersParamIndex = parent->registerArrayParam(cmdData.outResourceTables);
    m_debugNameMarkerParamIndex = parent->registerArrayParam(cmdData.debugName);
}

void ComputeCommand::setShader(ShaderHandle shader)
{
    auto& cmdData = *data<AbiComputeCmd>();
    cmdData.shader = shader;
}

void ComputeCommand::setConstants(Buffer* constBuffers, int bufferCounts)
{
    m_parent->storeArrayParam(m_constantBuffersParamIndex, constBuffers, bufferCounts);
}

void ComputeCommand::setInResources(InResourceTable* inTables, int tablesCount)
{
    m_parent->storeArrayParam(m_inputResourcesParamIndex, inTables, tablesCount);
}

void ComputeCommand::setOutResources(OutResourceTable* outTables, int tablesCount)
{
    m_parent->storeArrayParam(m_outputResourcesParamIndex, outTables, tablesCount);
}

void ComputeCommand::setDispatch(const char* debugNameMarker, int x, int y, int z)
{
    auto& cmdData = *data<AbiComputeCmd>();
    cmdData.x = x;
    cmdData.y = y;
    cmdData.z = z;
    if (debugNameMarker)
    {
        int charLen = strlen(debugNameMarker) + 1;
        m_parent->storeArrayParam(m_debugNameMarkerParamIndex, debugNameMarker, charLen);
        cmdData.debugNameSize = charLen;
    }
}

void CopyCommand::setResources(ResourceHandle source, ResourceHandle destination)
{
    auto& cmdData = *data<AbiCopyCmd>();
    cmdData.source = source;
    cmdData.destination = destination;
}

UploadCommand::UploadCommand(MemOffset offset, InternalCommandList* parent)
: GpuCommand(offset, parent)
{
    auto& cmdData = *data<AbiUploadCmd>();
    m_sourceParamIndex = parent->registerArrayParam(cmdData.sources);
}

void UploadCommand::setData(const void* source, int sourceSize, ResourceHandle destination)
{
    auto& cmdData = *data<AbiUploadCmd>();
    m_parent->storeArrayParam(m_sourceParamIndex, (const char*)source, sourceSize);
}

void DownloadCommand::setData(ResourceHandle source)
{
    auto& cmdData = *data<AbiDownloadCmd>();
    cmdData.source = source;
}

}
}
