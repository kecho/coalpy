#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.core/ByteBuffer.h>

namespace coalpy
{
namespace render
{

typedef uint64_t MemOffset;
typedef uint64_t MemSize;

class GpuCommand
{
protected:
    GpuCommand(MemOffset offset, ByteBuffer* buffer)
    : m_offset(offset), m_buffer(buffer)
    {
    }
    
    GpuCommand()
    : m_offset(0ull), m_buffer(nullptr)
    {
    }

    template<typename AbiType>
    AbiType* data()
    {
        return (AbiType*)(m_buffer->data() + m_offset);
    }

    MemOffset m_offset;
    ByteBuffer* m_buffer;
};

class ComputeCommand : private GpuCommand
{
    friend class CommandList;
public:
    inline void setConstants(Buffer constBuffer) { setConstants(&constBuffer, 1); }
    inline void setInResources(InResourceTable inTables) { setInResources(&inTables, 1); }
    inline void setOutResources(OutResourceTable outTables) { setOutResources(&outTables, 1); }
    void setShader(ShaderHandle shader);
    void setConstants(Buffer* constBuffers, int bufferCounts);
    void setInResources(InResourceTable* inTables, int inputTablesCount);
    void setOutResources(OutResourceTable* outTables, int inputTablesCount);
    void setDispatch(const char* debugNameMarker, int x, int y, int z);

private:
    ComputeCommand() {}
    ComputeCommand(MemOffset offset, ByteBuffer* buffer)
        : GpuCommand(offset, buffer)
    {
    }
};

class CopyCommand : private GpuCommand
{
    friend class CommandList;
public:
    void setResources(ResourceHandle source, ResourceHandle destination);

private:
    CopyCommand() {}
    CopyCommand(MemOffset offset, ByteBuffer* buffer)
        : GpuCommand(offset, buffer)
    {
    }
};

struct UploadCommand : private GpuCommand
{
    friend class CommandList;
public:
    void setData(const void* source, int sourceSize, ResourceHandle destination);

private:
    UploadCommand() {}
    UploadCommand(MemOffset offset, ByteBuffer* buffer)
        : GpuCommand(offset, buffer)
    {
    }
};

struct DownloadCommand : private GpuCommand
{
    friend class CommandList;
public:
    void setData(ResourceHandle source, void* destinationBuffer, int destinationSize);

private:
    DownloadCommand() {}
    DownloadCommand(MemOffset offset, ByteBuffer* buffer)
    : GpuCommand(offset, buffer)
    {
    }
};

class CommandList
{
public:
    CommandList();
    ~CommandList();

    ComputeCommand   addComputeCommand();
    CopyCommand      addCopyCommand();
    UploadCommand    addUploadCommand();
    DownloadCommand  addDownloadCommand();
    void close();

    const ByteBuffer& data() const { return m_buffer; }

private:
    template<typename AbiType>
    MemOffset allocate();
    ByteBuffer m_buffer;
    bool m_closed;
};


}
}
