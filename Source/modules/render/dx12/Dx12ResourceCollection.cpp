#pragma once

#include <Config.h>

#if ENABLE_DX12

#include "Dx12ResourceCollection.h"
#include "Dx12Device.h"
#include "WorkBundleDb.h"
#include <coalpy.core/Assert.h>
#include <vector>

namespace coalpy
{
namespace render
{

Dx12ResourceCollection::Dx12ResourceCollection(Dx12Device& device, WorkBundleDb& workDb)
: m_device(device)
, m_workDb(workDb)
{
}

Dx12ResourceCollection::~Dx12ResourceCollection()
{
    m_workDb.clearAllTables();
    m_workDb.clearAllResources();
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
    m_workDb.registerResource(resHandle, desc.memFlags, outPtr->resource->defaultGpuState());
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
    m_workDb.registerResource(resHandle, desc.memFlags, outPtr->resource->defaultGpuState());
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

        auto& config = container->resource->config();
        if (((config.memFlags & MemFlag_GpuRead) == 0 && !isUav)
         || ((config.memFlags & MemFlag_GpuWrite) == 0 && isUav))
        {
            CPY_ASSERT_FMT(false, "Tried to create resource table %s but memflags are incorrect:%d",
                (desc.name.empty() ? "<no-name>" : desc.name.c_str()),
                config.memFlags);
            return ResourceTable();
        }

        gatheredResources.push_back(&(*container->resource));
    }

    ResourceTable handle;
    auto& outPtr = m_resourceTables.allocate(handle);
    outPtr = new Dx12ResourceTable(m_device, gatheredResources.data(), (int)gatheredResources.size(), isUav); 
    return handle;
}

InResourceTable Dx12ResourceCollection::createInResourceTable(const ResourceTableDesc& desc)
{
    ResourceTable handle = createResourceTable(desc, false);
    m_workDb.registerTable(handle, desc.resources, desc.resourcesCount, false);
    return InResourceTable { handle.handleId };
}

OutResourceTable Dx12ResourceCollection::createOutResourceTable(const ResourceTableDesc& desc)
{
    ResourceTable handle = createResourceTable(desc, true);
    m_workDb.registerTable(handle, desc.resources, desc.resourcesCount, true);
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
    m_workDb.unregisterResource(handle);
}

void Dx12ResourceCollection::release(ResourceTable handle)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = handle.valid() && m_resourceTables.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;
    m_resourceTables.free(handle);
    m_workDb.unregisterTable(handle);
}

}
}

#endif
