#pragma once

#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include "Dx12Resources.h"
#include <mutex>

struct ID3D12Resource;

namespace coalpy
{
namespace render
{

class Dx12Device;

class Dx12ResourceCollection
{
public:
    Dx12ResourceCollection(Dx12Device& device);
    ~Dx12ResourceCollection();

    Texture createTexture(const TextureDesc& desc, ID3D12Resource* resourceToAcquire = nullptr);
    Buffer  createBuffer (const BufferDesc& desc, ID3D12Resource* resourceToAcquire = nullptr);
    InResourceTable  createInResourceTable(const ResourceTableDesc& desc);
    OutResourceTable createOutResourceTable(const ResourceTableDesc& desc);

    void release(ResourceHandle resource);
    void release(ResourceTable resource);

private:
    ResourceTable createResourceTable(const ResourceTableDesc& desc, bool isUav);
    enum class ResType { Texture, Buffer };
    class ResourceContainer : public RefCounted
    {
    public:
        ResType type = ResType::Texture;
        SmartPtr<Dx12Resource> resource;
    };

    Dx12Device& m_device;
    std::mutex m_resourceMutex;
    HandleContainer<ResourceHandle, SmartPtr<ResourceContainer>> m_resources;
    HandleContainer<ResourceTable , SmartPtr<Dx12ResourceTable>> m_resourceTables;
};

}
}
