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

struct Texture  : public ResourceHandle {};
struct Buffer   : public ResourceHandle {};
struct Sampler  : public ResourceHandle {};
struct InResourceTable  : public ResourceTable {};
struct OutResourceTable : public ResourceTable {};
struct SamplerTable     : public ResourceTable {};

enum class TextureType
{
    k1d,
    k2d,
    k2dArray,
    k3d,
    CubeMap,
    CubeMapArray,
    Count
};

enum MemFlags : int
{
    MemFlag_GpuRead  = 1 << 0,
    MemFlag_GpuWrite = 1 << 1,
};

enum class BufferType
{
    Standard,
    Structured,
    Raw,
    Count
};

enum class TextureAddressMode
{
    Wrap, Mirror, Clamp, Border, Count
};

enum class FilterType
{
    Point, Linear, Min, Max, Anisotropic
};

struct SamplerDesc
{
    FilterType type = FilterType::Linear;
    TextureAddressMode addressU = TextureAddressMode::Wrap;
    TextureAddressMode addressV = TextureAddressMode::Wrap;
    TextureAddressMode addressW = TextureAddressMode::Wrap;
    float borderColor [4];
    float mipBias = 0.0f;
    float minLod = 0.0f;
    float maxLod = 16.0f;
    int maxAnisoQuality = 2;
};

struct ResourceDesc
{
    ResourceName name;
    MemFlags memFlags = (MemFlags)(MemFlag_GpuWrite | MemFlag_GpuRead);
    bool recreatable = false;
};

struct TextureDesc : public ResourceDesc
{
    TextureType type = TextureType::k2d;
    Format format = Format::RGBA_8_UINT;
    unsigned int width = 1u;
    unsigned int height = 1u;
    unsigned int depth  = 1u;
    unsigned int mipLevels = 1u;
    bool isRtv = false;
};

struct BufferDesc : public ResourceDesc
{
    BufferType type = BufferType::Standard;
    Format format = Format::RGBA_32_SINT;
    bool isConstantBuffer = false;
    int elementCount  = 1;
    int stride = 4;
};

struct ResourceTableDesc
{
    ResourceName name;
    const ResourceHandle* resources = nullptr;
    int resourcesCount = 0;
};

enum class ResourceResult
{
    Ok,
    InvalidParameter,
    InvalidHandle,
    InternalApiFailure
};

template<typename ResourceType>
struct TemplateResourceResult 
{
    bool success() const { return result == ResourceResult::Ok; }

    ResourceResult result = ResourceResult::Ok;
    ResourceType object;
    std::string message;

    operator ResourceType() const
    {
        return object;
    }
};

using TextureResult = TemplateResourceResult<Texture>;
using BufferResult  = TemplateResourceResult<Buffer>;
using SamplerResult = TemplateResourceResult<Sampler>;
using InResourceTableResult  = TemplateResourceResult<InResourceTable>;
using OutResourceTableResult = TemplateResourceResult<OutResourceTable>;
using SamplerTableResult  = TemplateResourceResult<SamplerTable>;

}

}

namespace std
{

template<>
struct hash<coalpy::render::ResourceHandle>
{
    size_t operator()(const coalpy::render::ResourceHandle h) const
    {
        return (size_t)h.handleId;
    }
};

template<>
struct hash<coalpy::render::Texture>
{
    size_t operator()(const coalpy::render::Texture& h) const
    {
        return (std::size_t)h.handleId;
    }
};

template<>
struct hash<coalpy::render::Buffer>
{
    size_t operator()(const coalpy::render::Buffer& h) const
    {
        return (std::size_t)h.handleId;
    }
};

template<>
struct hash<coalpy::render::ResourceTable>
{
    size_t operator()(const coalpy::render::ResourceTable h) const
    {
        return (size_t)h.handleId;
    }
};

}
