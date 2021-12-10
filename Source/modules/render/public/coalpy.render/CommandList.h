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

    inline void setInlineConstant(const char* buffer, int bufferSize)
    {
        m_constBuffers = nullptr;
        m_constBuffersCounts = 0;
        m_inlineConstantBuffer = buffer;
        m_inlineConstantBufferSize = bufferSize;
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

    inline void setSamplers(SamplerTable* samplerTables, int tablesCounts)
    {
        m_samplerTables = samplerTables;
        m_samplerTablesCounts = tablesCounts;
    }

    inline void setDispatch(const char* debugNameMarker, int x, int y, int z)
    {
        m_isIndirect = false;
        m_debugName = debugNameMarker;
        m_x = x;
        m_y = y;
        m_z = z;
    }

    inline void setIndirectDispatch(const char* debugNameMarker, Buffer argumentBuffer)
    {
        m_isIndirect = true;
        m_debugName = debugNameMarker;
        m_argumentBuffer = argumentBuffer;
    }

private:
    ShaderHandle m_shader;

    const Buffer* m_constBuffers;
    int m_constBuffersCounts = 0;

    const InResourceTable* m_inTables;
    int m_inTablesCounts = 0;

    const OutResourceTable* m_outTables;
    int m_outTablesCounts = 0;

    const SamplerTable* m_samplerTables;
    int m_samplerTablesCounts = 0;

    const char* m_inlineConstantBuffer = nullptr;
    int m_inlineConstantBufferSize = 0;

    const char* m_debugName = "";
    int m_x = 1;
    int m_y = 1;
    int m_z = 1;

    bool m_isIndirect = false;
    Buffer m_argumentBuffer;
};

class CopyCommand
{
    friend class CommandList;
public:
    void setResources(ResourceHandle source, ResourceHandle destination)
    {
        m_fullCopy = true;
        m_source = source;
        m_destination = destination;
    }

    void setBuffers(Buffer source, Buffer destination, int byteSize = -1, int sourceByteOffset = 0, int destinationByteOffset = 0)
    {
        m_fullCopy = false;
        m_source = source;
        m_destination = destination;
        m_sourceX = sourceByteOffset;
        m_destX = destinationByteOffset;
        m_sizeX = byteSize;
    }

    void setTextures(Texture source, Texture destination,
        int sizeX = -1, int sizeY = -1, int sizeZ = -1,
        int sourceX = 0,
        int sourceY = 0,
        int sourceZ = 0,
        int destX = 0,
        int destY = 0,
        int destZ = 0,
        int srcMipLevel = 0,
        int dstMipLevel = 0)
    {
        m_fullCopy = false;
        m_source = source;
        m_destination = destination;
        m_sourceX = sourceX;
        m_sourceY = sourceY;
        m_sourceZ = sourceZ;
        m_destX = destX;
        m_destY = destY;
        m_destZ = destZ;
        m_sizeX = sizeX;
        m_sizeY = sizeY;
        m_sizeZ = sizeZ;
        m_srcMipLevel = srcMipLevel;
        m_dstMipLevel = dstMipLevel;
    }

private:
    ResourceHandle m_source;
    ResourceHandle m_destination;

    int m_sourceX = 0;
    int m_sourceY = 0;
    int m_sourceZ = 0;

    int m_destX = 0;
    int m_destY = 0;
    int m_destZ = 0;

    int m_sizeX = -1;
    int m_sizeY = -1;
    int m_sizeZ = -1;

    int m_srcMipLevel = 0;
    int m_dstMipLevel = 0;

    bool m_fullCopy = true;
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

    void setBufferDestOffset(int offset)
    {
        m_destX = offset;
    }

    void setTextureDestInfo(
        int sizeX, int sizeY, int sizeZ,
        int destX, int destY, int destZ, int mipLevel)
    {
        m_mipLevel = mipLevel;
        m_sizeX = sizeX;
        m_sizeY = sizeY;
        m_sizeZ = sizeZ;
        m_destX = destX;
        m_destY = destY;
        m_destZ = destZ;
    }

private:
    const char* m_source = nullptr;
    int m_sourceSize = 0;
    int m_mipLevel = 0;
    int m_arraySlice = 0;
    int m_sizeX = -1;
    int m_sizeY = -1;
    int m_sizeZ = -1;
    int m_destX = 0;
    int m_destY = 0;
    int m_destZ = 0;
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

    void setMipLevel(int mipLevel)
    {
        m_mipLevel = mipLevel;
    }

    void setArraySlice(int arraySlice)
    {
        m_arraySlice = arraySlice;
    }

private:
    ResourceHandle m_source;
    int m_arraySlice = 0;
    int m_mipLevel = 0;
};

struct ClearAppendConsumeCounter
{
    friend class CommandList;
public:
    void setData(ResourceHandle source, int counter = 0)
    {
        m_source = source;
        m_counter = counter;
    }

private:
    ResourceHandle m_source;
    int m_counter;
};

class InternalCommandList;

class CommandList
{
public:
    CommandList();
    ~CommandList();

    void beginMarker(const char* name);
    void endMarker();
    void writeCommand(const ComputeCommand& cmd);
    void writeCommand(const CopyCommand& cmd);
    void writeCommand(const UploadCommand& cmd);
    void writeCommand(const DownloadCommand& cmd);
    void writeCommand(const ClearAppendConsumeCounter& cmd);

    MemOffset uploadInlineResource(ResourceHandle destination, int sourceSize);

    void reset();
    void finalize();

    bool isFinalized() const;
    const unsigned char* data() const;
    unsigned char* data();
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
