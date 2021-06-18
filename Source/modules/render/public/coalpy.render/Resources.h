#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/Formats.h>

namespace coalpy
{

namespace render
{

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

struct TextureDesc
{
    TextureType type = TextureType::k2d;
    Format format = Format::RGBA_8_UINT;
    unsigned int width = 1u;
    unsigned int height = 1u;
    unsigned int mipLevels = 1u;
    MemFlags flags = GpuWrite;
};

struct Texture : GenericHandle<unsigned int> {};

}

}
