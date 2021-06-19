#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/Formats.h>
#include <sstream>

namespace coalpy
{

namespace render
{

using ResourceName = std::stringstream;

struct ResourceHandle : public GenericHandle<unsigned int> {};
struct ResourceTable  : public GenericHandle<unsigned int> {};

struct Texture : public ResourceHandle {};
struct Buffer  : public ResourceHandle {};
struct InResourceTable  : public ResourceTable {};
struct OutResourceTable : public ResourceTable {};

enum class TextureType
{
    k1d,
    k2d,
    k2dArray,
    k3d,
    CubeMap,
    CubeMapArray
};

enum MemFlags
{
    CpuRead  = 1 << 0,
    CpuWrite = 1 << 1,
    GpuWrite = 1 << 2,
};

enum class BufferType
{
    Standard,
    Structured
};

struct TextureDesc
{
    ResourceName name;
    TextureType type = TextureType::k2d;
    Format format = Format::RGBA_8_UINT;
    unsigned int width = 1u;
    unsigned int height = 1u;
    unsigned int depth  = 1u;
    unsigned int mipLevels = 1u;
    MemFlags flags = GpuWrite;
};

struct BufferDesc
{
    ResourceName name;
    BufferType type = BufferType::Standard;
    Format format = Format::RGBA_8_UINT;
    int elementsCount  = 1;
    int structuredBufferStride = -1;
    MemFlags flags = GpuWrite;
};

struct ResourceTableDesc
{
    ResourceName name;
    ResourceHandle* resources = nullptr;
    int resourcesCount = 0;
};

}

}
