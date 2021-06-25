#pragma once

#include <Config.h>

#if ENABLE_DX12

#include "Dx12ResourceCollection.h"
#include "Dx12Device.h"
#include <coalpy.core/Assert.h>
#include <vector>

namespace coalpy
{
namespace render
{

Dx12ResourceCollection::Dx12ResourceCollection(Dx12Device& device)
: m_device(device)
{
}

Dx12ResourceCollection::~Dx12ResourceCollection()
{
}

Texture Dx12ResourceCollection::createTexture(const TextureDesc& desc, ID3D12Resource* resource)
{
    std::unique_lock lock(m_resourceMutex);
    ResourceHandle resHandle;
    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();
    CPY_ASSERT(outPtr->resource == nullptr);
    
    outPtr->type = ResType::Texture;
    outPtr->resource = new Dx12Texture(m_device, desc);
    if (resource)
        outPtr->resource->acquireD3D12Resource(resource);
    outPtr->resource->init();
    return Texture { resHandle.handleId };
}

Buffer Dx12ResourceCollection::createBuffer(const BufferDesc& desc, ID3D12Resource* resource)
{
    std::unique_lock lock(m_resourceMutex);
    ResourceHandle resHandle;
    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();
    CPY_ASSERT(outPtr->resource == nullptr);
    
    outPtr->type = ResType::Buffer;
    outPtr->resource = new Dx12Buffer(m_device, desc);
    if (resource)
        outPtr->resource->acquireD3D12Resource(resource);
    outPtr->resource->init();
    return Buffer { resHandle.handleId };
}

ResourceTable Dx12ResourceCollection::createResourceTable(const ResourceTableDesc& desc, bool isUav)
{
    std::unique_lock lock(m_resourceMutex);
    std::vector<Dx12Resource*> gatheredResources;
    for (int i = 0; i < desc.resourcesCount; ++i)
    {
        ResourceHandle h = desc.resources[i];
        bool foundResource = m_resources.contains(h);
        CPY_ASSERT(foundResource);
        if (!foundResource)
            return ResourceTable();

        SmartPtr<ResourceContainer>& container = m_resources[h];
        CPY_ASSERT(container->resource != nullptr);
        if (container->resource == nullptr)
            return ResourceTable();

        gatheredResources.push_back(&(*container->resource));
    }

    ResourceTable handle;
    auto& outPtr = m_resourceTables.allocate(handle);
    outPtr = new Dx12ResourceTable(m_device, gatheredResources.data(), (int)gatheredResources.size(), isUav); 
    return handle;
}

InResourceTable Dx12ResourceCollection::createInputResourceTable(const ResourceTableDesc& desc)
{
    ResourceTable handle = createResourceTable(desc, false);
    return InResourceTable { handle.handleId };
}

OutResourceTable Dx12ResourceCollection::createOutputResourceTable(const ResourceTableDesc& desc)
{
    ResourceTable handle = createResourceTable(desc, true);
    return OutResourceTable { handle.handleId };
}

void Dx12ResourceCollection::release(ResourceHandle handle)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = handle.valid() && m_resources.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;
    (*m_resources[handle]) = {};
    m_resources.free(handle, false);
}

}
}

#endif
