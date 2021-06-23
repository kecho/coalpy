#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/Formats.h>
#include <string>

namespace coalpy
{

namespace render
{

using ResourceName = std::string;

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

enum MemFlags : int
{
    MemFlag_CpuRead  = 1 << 0,
    MemFlag_CpuWrite = 1 << 1,
    MemFlag_GpuRead  = 1 << 2,
    MemFlag_GpuWrite = 1 << 3
};

enum class BufferType
{
    Standard,
    Structured
};

struct ResourceDesc
{
    ResourceName name;
    MemFlags memFlags;
};

struct TextureDesc : public ResourceDesc
{
    TextureType type = TextureType::k2d;
    Format format = Format::RGBA_8_UINT;
    unsigned int width = 1u;
    unsigned int height = 1u;
    unsigned int depth  = 1u;
    unsigned int mipLevels = 1u;
};

struct BufferDesc : public ResourceDesc
{
    BufferType type = BufferType::Standard;
    Format format = Format::RGBA_8_UINT;
    int elementsCount  = 1;
    int structuredBufferStride = -1;
};

struct ResourceTableDesc
{
    ResourceName name;
    ResourceHandle* resources = nullptr;
    int resourcesCount = 0;
};

}

}
