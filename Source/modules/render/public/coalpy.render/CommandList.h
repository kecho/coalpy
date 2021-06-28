#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.render/ShaderDefs.h>

namespace coalpy
{
namespace render
{

typedef uint64_t MemOffset;
typedef uint64_t MemSize;

class InternalCommandList;

class GpuCommand
{
protected:
    GpuCommand(MemOffset offset, InternalCommandList* parent)
    : m_offset(offset), m_parent(parent)
    {
    }
    
    GpuCommand()
    : m_offset(0ull), m_parent(nullptr)
    {
    }

    template<typename AbiType>
    AbiType* data()
    {
        return (AbiType*)(m_buffer->data() + m_offset);
    }

    MemOffset m_offset;
    InternalCommandList* m_parent;
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
    int m_inputResourcesParamIndex = -1;
    int m_outputResourcesParamIndex = -1;
    int m_constantBuffersParamIndex = -1;
    int m_debugNameMarkerParamIndex = -1;
    ComputeCommand() {}
    ComputeCommand(MemOffset offset, InternalCommandList* parent);
};

class CopyCommand : private GpuCommand
{
    friend class CommandList;
public:
    void setResources(ResourceHandle source, ResourceHandle destination);

private:
    CopyCommand() {}
    CopyCommand(MemOffset offset, InternalCommandList* parent)
        : GpuCommand(offset, parent)
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
    UploadCommand(MemOffset offset, InternalCommandList* parent);

    int m_sourceParamIndex = -1;
};

struct DownloadCommand : private GpuCommand
{
    friend class CommandList;
public:
    void setData(ResourceHandle source);

private:
    DownloadCommand() {}
    DownloadCommand(MemOffset offset, InternalCommandList* parent)
    : GpuCommand(offset, parent)
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

    unsigned char* data();
    size_t size() const;
    void close();

private:
    template<typename AbiType>
    MemOffset allocate();
    InternalCommandList& m_internal;
};


}
}
