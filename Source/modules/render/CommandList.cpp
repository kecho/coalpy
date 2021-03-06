#include <coalpy.render/CommandList.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/Assert.h>
#include <cstring>
#include <vector>

namespace coalpy
{
namespace render
{

struct CmdPendingMemory
{
    const u8* src = nullptr;
    MemSize srcByteSize = 0;
    MemOffset destinationOffset; //offset to pointer / size blob member
};

class InternalCommandList
{
public:
    ByteBuffer buffer;
    std::vector<CmdPendingMemory> pendingMemory;
    bool closed = false;

    void reset()
    {
        buffer.resize(0);
        pendingMemory.clear();
        closed = false;
    }
    template<typename ElementType>
    void deferArrayStore(AbiPtr<ElementType>& param, const ElementType* srcArray, int counts)
    {
        if (counts == 0)
            return;

        pendingMemory.emplace_back();
        auto& pendingMem = pendingMemory.back();
        pendingMem.destinationOffset = (u8*)&param.offset - buffer.data();
        pendingMem.src         = (const u8*)srcArray;
        pendingMem.srcByteSize = (MemSize)(sizeof(ElementType) * counts);
    }
};

CommandList::CommandList()
: m_internal(*(new InternalCommandList()))
{
    reset();
}

CommandList::~CommandList()
{
    delete &m_internal;
}

const u8* CommandList::data() const
{
    CPY_ASSERT_MSG(m_internal.closed, "Command list has not been finalized. Data should not be accessed.");
    return m_internal.buffer.data();
}

u8* CommandList::data()
{
    CPY_ASSERT_MSG(m_internal.closed, "Command list has not been finalized. Data should not be accessed.");
    return m_internal.buffer.data();
}

size_t CommandList::size() const
{
    return m_internal.buffer.size();
}

void CommandList::flushDeferredStores()
{
    for (auto& pendingMem : m_internal.pendingMemory)
    {
        MemOffset currOffset = (MemOffset)m_internal.buffer.size();
        u8* dataPtr = m_internal.buffer.data();
        CPY_ASSERT(pendingMem.destinationOffset <= (currOffset - sizeof(MemOffset)));
        *((MemOffset*)(dataPtr + pendingMem.destinationOffset)) = currOffset;
        m_internal.buffer.append(pendingMem.src, pendingMem.srcByteSize);
    }

    m_internal.pendingMemory.clear();
}

void CommandList::finalize()
{
    if (m_internal.closed)
        return;

    int endSentinel = (int)AbiCmdTypes::CommandListEndSentinel;
    m_internal.buffer.append(&endSentinel);

    //store the size
    AbiCommandListHeader& header = *((AbiCommandListHeader*)(m_internal.buffer.data()));
    header.commandListSize = m_internal.buffer.size();

    m_internal.closed = true;
}

bool CommandList::isFinalized() const
{
    return m_internal.closed;
}

template<typename AbiType>
AbiType& CommandList::allocate()
{
    auto& buffer = m_internal.buffer;
    auto offset = (MemOffset)buffer.size();
    buffer.appendEmpty(sizeof(AbiType));
    auto* abiObj = (AbiType*)(buffer.data() + offset);
    new (abiObj) AbiType;
    return *abiObj;
}

template<typename AbiType>
void CommandList::finalizeCommand(AbiType& t)
{
    MemOffset offset = (MemOffset)((const u8*)&t - m_internal.buffer.data());
    flushDeferredStores();

    AbiType* obj = (AbiType*)(m_internal.buffer.data() + offset);
    obj->cmdSize = m_internal.buffer.size() - offset;
}

void CommandList::writeCommand(const ComputeCommand& cmd)
{
    auto& abiCmd = allocate<AbiComputeCmd>();

    abiCmd.shader = cmd.m_shader;

    m_internal.deferArrayStore(abiCmd.constants, cmd.m_constBuffers, cmd.m_constBuffersCounts);
    abiCmd.constantCounts = cmd.m_constBuffersCounts;

    m_internal.deferArrayStore(abiCmd.inlineConstantBuffer, cmd.m_inlineConstantBuffer, cmd.m_inlineConstantBufferSize);
    abiCmd.inlineConstantBufferSize = cmd.m_inlineConstantBufferSize;

    m_internal.deferArrayStore(abiCmd.inResourceTables, cmd.m_inTables, cmd.m_inTablesCounts);
    abiCmd.inResourceTablesCounts = cmd.m_inTablesCounts;

    m_internal.deferArrayStore(abiCmd.outResourceTables, cmd.m_outTables, cmd.m_outTablesCounts);
    abiCmd.outResourceTablesCounts = cmd.m_outTablesCounts;

    m_internal.deferArrayStore(abiCmd.samplerTables, cmd.m_samplerTables, cmd.m_samplerTablesCounts);
    abiCmd.samplerTablesCounts = cmd.m_samplerTablesCounts;

    const char* str = cmd.m_debugName ? cmd.m_debugName : "";
    int strSz = strlen(cmd.m_debugName) + 1;
    m_internal.deferArrayStore(abiCmd.debugName, cmd.m_debugName, strSz);

    abiCmd.debugNameSize = strSz;

    if (cmd.m_isIndirect)
    {
        abiCmd.isIndirect = 1;
        abiCmd.indirectArguments = cmd.m_argumentBuffer;
    }
    else
    {
        abiCmd.isIndirect = 0;
        abiCmd.x = cmd.m_x;
        abiCmd.y = cmd.m_y;
        abiCmd.z = cmd.m_z;
    }

    finalizeCommand(abiCmd);
}

void CommandList::reset()
{
    m_internal.reset();
    allocate<AbiCommandListHeader>();
}

void CommandList::writeCommand(const CopyCommand& cmd)
{
    auto& abiCmd = allocate<AbiCopyCmd>();
    abiCmd.fullCopy = cmd.m_fullCopy ? 1 : 0;
    abiCmd.source = cmd.m_source;
    abiCmd.destination = cmd.m_destination;
    abiCmd.sourceX = cmd.m_sourceX;
    abiCmd.sourceY = cmd.m_sourceY;
    abiCmd.sourceZ = cmd.m_sourceZ;
    abiCmd.destX = cmd.m_destX;
    abiCmd.destY = cmd.m_destY;
    abiCmd.destZ = cmd.m_destZ;
    abiCmd.sizeX = cmd.m_sizeX;
    abiCmd.sizeY = cmd.m_sizeY;
    abiCmd.sizeZ = cmd.m_sizeZ;
    abiCmd.srcMipLevel = cmd.m_srcMipLevel;
    abiCmd.dstMipLevel = cmd.m_dstMipLevel;
    finalizeCommand(abiCmd);
}

void CommandList::writeCommand(const UploadCommand& cmd)
{
    auto& abiCmd = allocate<AbiUploadCmd>();
    abiCmd.destination = cmd.m_destination;
    m_internal.deferArrayStore(abiCmd.sources, cmd.m_source, cmd.m_sourceSize);
    abiCmd.sourceSize = cmd.m_sourceSize;
    abiCmd.sizeX = cmd.m_sizeX;
    abiCmd.sizeY = cmd.m_sizeY;
    abiCmd.sizeZ = cmd.m_sizeZ;
    abiCmd.destX = cmd.m_destX;
    abiCmd.destY = cmd.m_destY;
    abiCmd.destZ = cmd.m_destZ;
    abiCmd.mipLevel = cmd.m_mipLevel;
    finalizeCommand(abiCmd);
}

void CommandList::writeCommand(const DownloadCommand& cmd)
{
    auto& abiCmd = allocate<AbiDownloadCmd>();
    abiCmd.source = cmd.m_source;   
    abiCmd.mipLevel = cmd.m_mipLevel;
    abiCmd.arraySlice = cmd.m_arraySlice;
    finalizeCommand(abiCmd);
}

void CommandList::writeCommand(const ClearAppendConsumeCounter& cmd)
{
    auto& abiCmd = allocate<AbiClearAppendConsumeCounter>();
    abiCmd.source = cmd.m_source;
    abiCmd.counter = cmd.m_counter;
    finalizeCommand(abiCmd);
}

void CommandList::writeCommand(const CopyAppendConsumeCounterCommand& cmd)
{
    auto& abiCmd = allocate<AbiCopyAppendConsumeCounter>();
    abiCmd.source = cmd.m_source;
    abiCmd.destination = cmd.m_destination;
    abiCmd.destinationOffset = cmd.m_destinationOffset;
    finalizeCommand(abiCmd);
}

void CommandList::beginMarker(const char* name)
{
    auto& abiCmd = allocate<AbiBeginMarker>();
    if (name == nullptr)
        m_internal.deferArrayStore(abiCmd.str, "", 1);
    else
        m_internal.deferArrayStore(abiCmd.str, name, strlen(name) + 1);
    finalizeCommand(abiCmd);
}

void CommandList::endMarker()
{
    auto& abiCmd = allocate<AbiEndMarker>();
    finalizeCommand(abiCmd);
}

MemOffset CommandList::uploadInlineResource(ResourceHandle destination, int sourceSize)
{
    MemOffset cmdOffset = m_internal.buffer.size();
    allocate<AbiUploadCmd>();

    MemOffset dataOffset = m_internal.buffer.size();
    m_internal.buffer.appendEmpty(sourceSize);

    AbiUploadCmd& uploadCmd = *(AbiUploadCmd*)(m_internal.buffer.data() + cmdOffset);
    uploadCmd.cmdSize = m_internal.buffer.size() - cmdOffset;
    uploadCmd.destination = destination;
    uploadCmd.sources.offset = dataOffset;
    uploadCmd.sourceSize = sourceSize;
    return dataOffset;
}


}
}
