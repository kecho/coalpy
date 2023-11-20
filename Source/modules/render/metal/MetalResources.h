#pragma once

#include <mutex>
#include <set>
#include <unordered_map>
#include <coalpy.render/Resources.h>
#include <coalpy.core/HandleContainer.h>

@protocol MTLBuffer;

namespace coalpy
{
namespace render
{

class MetalDevice;
class WorkBundleDb;

enum ResourceSpecialFlags : int
{
    ResourceSpecialFlag_None = 0,
    // ResourceSpecialFlag_NoDeferDelete = 1 << 0,
    // ResourceSpecialFlag_CanDenyShaderResources = 1 << 1,
    ResourceSpecialFlag_TrackTables = 1 << 2,
    // ResourceSpecialFlag_CpuReadback = 1 << 3,
    // ResourceSpecialFlag_CpuUpload = 1 << 4,
    // ResourceSpecialFlag_EnableColorAttachment = 1 << 5,
};

struct MetalResource
{
    enum class Type   
    {
        Buffer,
        Texture,
        Sampler
    };

    struct BufferData
    {
        id<MTLBuffer> mtlBuffer;
    };

    struct TextureData
    {
        id<MTLTexture> mtlTexture;
    };

    ResourceHandle handle;
    Type type = Type::Buffer;
    union
    {
        BufferData bufferData;
        TextureData textureData;
        // MTLSamplerState sampler;
    };

    ResourceSpecialFlags specialFlags = {};
    // TODO (Apoorva): Is this actually read anywhere?
    std::set<ResourceTable> trackedTables;
};

struct MetalResourceTable
{
    enum class Type
    {
        In, Out, Sampler
    };

    Type type = Type::In;
    std::unordered_map<ResourceHandle, int> trackedResources;
};

class MetalResources
{
public:
    enum 
    {
        // TODO (Apoorva): Check if this is a good number
        MaxResources = 4095
    };
    MetalResources(MetalDevice& device, WorkBundleDb& workDb);
    ~MetalResources();

    BufferResult createBuffer(const BufferDesc& desc, ResourceSpecialFlags specialFlags = ResourceSpecialFlag_None);
    TextureResult createTexture(const TextureDesc& desc);
    // SamplerResult  createSampler (const SamplerDesc& desc);
    InResourceTableResult createInResourceTable(const ResourceTableDesc& desc);
    OutResourceTableResult createOutResourceTable(const ResourceTableDesc& desc);

    void release(ResourceHandle handle);

private:
    std::mutex m_mutex;
    MetalDevice& m_device;
    WorkBundleDb& m_workDb;
    HandleContainer<ResourceHandle, MetalResource, MaxResources> m_container;
    HandleContainer<ResourceTable, MetalResourceTable, MaxResources> m_tables;
};

}
}