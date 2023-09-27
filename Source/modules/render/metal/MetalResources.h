#pragma once

#include <mutex>
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
    // ResourceSpecialFlag_TrackTables = 1 << 2,
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
    };

    ResourceHandle handle;
    Type type = Type::Buffer;
    union
    {
        BufferData bufferData;
        TextureData textureData;
        // MTLSamplerState sampler;
    };
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

    void release(ResourceHandle handle);

private:
    std::mutex m_mutex;
    MetalDevice& m_device;
    WorkBundleDb& m_workDb;
    HandleContainer<ResourceHandle, MetalResource, MaxResources> m_container;
};

}
}