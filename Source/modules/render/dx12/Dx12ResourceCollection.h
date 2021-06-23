#pragma once

#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include "Dx12Resources.h"
#include <mutex>

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

    Texture createTexture(const TextureDesc& desc);
    Buffer  createBuffer (const BufferDesc& desc);
    void release(ResourceHandle resource);

private:
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
};

}
}
