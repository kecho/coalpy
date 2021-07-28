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

TextureResult Dx12ResourceCollection::createTexture(const TextureDesc& desc, ID3D12Resource* resource, ResourceSpecialFlags flags)
{
    std::unique_lock lock(m_resourceMutex);

    SmartPtr<Dx12Resource> textureObj = new Dx12Texture(m_device, desc, flags);
    if (resource)
        textureObj->acquireD3D12Resource(resource);
    auto initResult = textureObj->init();
    if (!initResult.success())
        return TextureResult { initResult.result, Texture(), std::move(initResult.message) };

    ResourceHandle resHandle;
    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();
    CPY_ASSERT(outPtr->resource == nullptr);
    
    outPtr->type = ResType::Texture;
    outPtr->resource = textureObj;
    outPtr->handle = resHandle;
    m_workDb.registerResource(resHandle, desc.memFlags, outPtr->resource->defaultGpuState());
    return TextureResult { ResourceResult::Ok, Texture { resHandle.handleId } };
}

TextureResult Dx12ResourceCollection::recreateTexture(Texture handle, const TextureDesc& desc, ID3D12Resource* resourceToAcquire)
{
    std::unique_lock lock(m_resourceMutex);
    CPY_ASSERT(handle.valid())
    CPY_ASSERT(m_resources.contains(handle));
    if (!handle.valid() || !m_resources.contains(handle))
        return TextureResult { ResourceResult::InvalidHandle, Texture(), "Invalid handle on recreateTexture call." };

    auto& c = m_resources[handle];
    CPY_ASSERT(c->type == ResType::Texture);
    if (c->type != ResType::Texture)
        return TextureResult { ResourceResult::InvalidParameter, Texture(), "Used a texture handle, expected buffer handle." };

    CPY_ASSERT(c->handle == handle);
    if (c->handle != handle)
        return TextureResult { ResourceResult::InternalApiFailure, Texture(), "Handles don't match, internal error on recreation." };

    auto oldFlags = c->resource->specialFlags();
    SmartPtr<Dx12Resource> textureObj = new Dx12Texture(m_device, desc, oldFlags);
    if (resourceToAcquire)
        textureObj->acquireD3D12Resource(resourceToAcquire);
    auto initResult = textureObj->init();
    if (!initResult.success())
        return TextureResult { initResult.result, Texture(), std::move(initResult.message) };

    c->resource = textureObj; 
    return TextureResult { ResourceResult::Ok, handle };
}

BufferResult Dx12ResourceCollection::createBuffer(const BufferDesc& desc, ID3D12Resource* resource, ResourceSpecialFlags flags)
{
    std::unique_lock lock(m_resourceMutex);
    ResourceHandle resHandle;

    SmartPtr<Dx12Resource> bufferObj = new Dx12Buffer(m_device, desc, flags);
    if (resource)
        bufferObj->acquireD3D12Resource(resource);

    auto initResult = bufferObj->init();
    if (!initResult.success())
        return BufferResult { initResult.result, Buffer(), std::move(initResult.message) };

    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();
    CPY_ASSERT(outPtr->resource == nullptr);
    
    outPtr->type = ResType::Buffer;
    outPtr->resource = bufferObj;
    outPtr->handle = resHandle;
    m_workDb.registerResource(resHandle, desc.memFlags, outPtr->resource->defaultGpuState());
    return BufferResult { ResourceResult::Ok, Buffer { resHandle.handleId } };
}

bool Dx12ResourceCollection::convertTableDescToResourceList(
    const ResourceTableDesc& desc,
    std::vector<Dx12Resource*>& resources,
    std::set<ResourceContainer*>& trackedContainers, bool isUav)
{
    for (int i = 0; i < desc.resourcesCount; ++i)
    {
        ResourceHandle h = desc.resources[i];
        bool foundResource = m_resources.contains(h);
        CPY_ASSERT(foundResource);
        if (!foundResource)
            return false;

        SmartPtr<ResourceContainer>& container = m_resources[h];
        CPY_ASSERT(container->resource != nullptr);
        if (container->resource == nullptr)
            return false;

        auto& config = container->resource->config();
        if (((config.memFlags & MemFlag_GpuRead) == 0 && !isUav)
         || ((config.memFlags & MemFlag_GpuWrite) == 0 && isUav))
        {
            CPY_ASSERT_FMT(false, "Tried to create resource table %s but memflags are incorrect:%d",
                (desc.name.empty() ? "<no-name>" : desc.name.c_str()),
                config.memFlags);
            return false;
        }

        if ((container->resource->specialFlags() & ResourceSpecialFlag_TrackTables) != 0)
            trackedContainers.insert(&(*container));

        resources.push_back(&(*container->resource));
    }

    return true;
}

ResourceTable Dx12ResourceCollection::createResourceTable(const ResourceTableDesc& desc, bool isUav)
{
    std::unique_lock lock(m_resourceMutex);
    std::vector<Dx12Resource*> gatheredResources;
    std::set<ResourceContainer*> containersToTrack;
    if (!convertTableDescToResourceList(desc, gatheredResources, containersToTrack, isUav))
        return ResourceTable();

    ResourceTable handle;
    auto& outPtr = m_resourceTables.allocate(handle);
    outPtr = new Dx12ResourceTable(m_device, gatheredResources.data(), (int)gatheredResources.size(), isUav); 

    if (!containersToTrack.empty())
    {
        auto& it = m_trackedTableToResources[handle];
        for (auto& c : containersToTrack)
        {
            it.insert(c->handle);
            c->parentTables.insert(handle);
        }
    }

    return handle;
}

InResourceTable Dx12ResourceCollection::createInResourceTable(const ResourceTableDesc& desc)
{
    ResourceTable handle = createResourceTable(desc, false);
    m_workDb.registerTable(handle, desc.name.c_str(), desc.resources, desc.resourcesCount, false);
    return InResourceTable { handle.handleId };
}

OutResourceTable Dx12ResourceCollection::createOutResourceTable(const ResourceTableDesc& desc)
{
    ResourceTable handle = createResourceTable(desc, true);
    m_workDb.registerTable(handle, desc.name.c_str(), desc.resources, desc.resourcesCount, true);
    return OutResourceTable { handle.handleId };
}

void Dx12ResourceCollection::recreate(ResourceTable handle)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = handle.valid() && m_resourceTables.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;

    SmartPtr<Dx12ResourceTable>& tableSlot = m_resourceTables[handle];
    auto it = m_workDb.tableInfos().find(handle);
    CPY_ASSERT(it != m_workDb.tableInfos().end());
    if (it == m_workDb.tableInfos().end())
        return;

    const WorkTableInfo& info = it->second;
    ResourceTableDesc desc;
    desc.name = info.name;
    desc.resources = info.resources.data();
    desc.resourcesCount = (int)info.resources.size();
    
    std::vector<Dx12Resource*> gatheredResources;
    std::set<ResourceContainer*> containersToTrack;
    bool result = convertTableDescToResourceList(desc, gatheredResources, containersToTrack, info.isUav);
    CPY_ASSERT(result);
    if (!result)
        return;

    tableSlot = nullptr;
    tableSlot = new Dx12ResourceTable(m_device, gatheredResources.data(), (int)gatheredResources.size(), info.isUav);
}

void Dx12ResourceCollection::release(ResourceHandle handle)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = handle.valid() && m_resources.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;
    auto& container = m_resources[handle];
    for (auto& trackedTable : container->parentTables)
    {
        auto it = m_trackedTableToResources.find(trackedTable);
        CPY_ASSERT(it != m_trackedTableToResources.end());
        if (it == m_trackedTableToResources.end())
            continue;

        it->second.erase(handle);
    }

    *container = {};
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

    auto it = m_trackedTableToResources.find(handle);
    if (it != m_trackedTableToResources.end())
    {
        for (auto& resHandle : it->second)
            m_resources[resHandle]->parentTables.erase(handle);
        m_trackedTableToResources.erase(it);
    }

    m_resourceTables.free(handle);
    m_workDb.unregisterTable(handle);
}

void Dx12ResourceCollection::getParentTables(ResourceHandle resource, std::vector<ResourceTable>& outTables)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = resource.valid() && m_resources.contains(resource);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;

    auto& container = m_resources[resource];
    CPY_ASSERT_MSG((container->resource->specialFlags() & ResourceSpecialFlag_TrackTables) != 0, "Resource must have TrackTables flags in order to track its parent tables");
    outTables.insert(outTables.end(), container->parentTables.begin(), container->parentTables.end());
}

}
}

#endif
