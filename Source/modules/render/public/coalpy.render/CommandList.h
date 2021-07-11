#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.render/AbiCommands.h>

namespace coalpy
{
namespace render
{

class ComputeCommand
{
    friend class CommandList;
public:
    inline void setShader(ShaderHandle shader)
    {
        m_shader = shader;
    }

    inline void setConstants(Buffer* constBuffers, int bufferCounts)
    {
        m_constBuffers = constBuffers;
        m_constBuffersCounts = bufferCounts;
    }

    inline void setInResources(InResourceTable* inTables, int inTablesCounts)
    {
        m_inTables = inTables;
        m_inTablesCounts = inTablesCounts;
    }

    inline void setOutResources(OutResourceTable* outTables, int outTablesCounts)
    {
        m_outTables = outTables;
        m_outTablesCounts = outTablesCounts;
    }

    inline void setDispatch(const char* debugNameMarker, int x, int y, int z)
    {
        m_debugName = debugNameMarker;
        m_x = x;
        m_y = y;
        m_z = z;
    }

private:
    ShaderHandle m_shader;

    const Buffer* m_constBuffers;
    int m_constBuffersCounts = 0;

    const InResourceTable* m_inTables;
    int m_inTablesCounts = 0;

    const OutResourceTable* m_outTables;
    int m_outTablesCounts = 0;

    const char* m_debugName = "";
    int m_x = 1;
    int m_y = 1;
    int m_z = 1;
};

class CopyCommand
{
    friend class CommandList;
public:
    void setResources(ResourceHandle source, ResourceHandle destination)
    {
        m_source = source;
        m_destination = destination;
    }

private:
    ResourceHandle m_source;
    ResourceHandle m_destination;
};

struct UploadCommand
{
    friend class CommandList;
public:
    void setData(const char* source, int sourceSize, ResourceHandle destination)
    {
        m_source = source;
        m_sourceSize = sourceSize;
        m_destination = destination;
    }

private:
    const char* m_source;
    int m_sourceSize;
    ResourceHandle m_destination;
};

struct DownloadCommand
{
    friend class CommandList;
public:
    void setData(ResourceHandle source)
    {
        m_source = source;
    }

private:
    ResourceHandle m_source;
};

class InternalCommandList;

class CommandList
{
public:
    CommandList();
    ~CommandList();

    void writeCommand(const ComputeCommand& cmd);
    void writeCommand(const CopyCommand& cmd);
    void writeCommand(const UploadCommand& cmd);
    void writeCommand(const DownloadCommand& cmd);
    void finalize();

    bool isFinalized() const;
    const unsigned char* data() const;
    size_t size() const;

private:
    template<typename AbiType>
    AbiType& allocate();

    template<typename AbiType>
    void finalizeCommand(AbiType& t);

    InternalCommandList& m_internal;
    void flushDeferredStores();
};


}
}
