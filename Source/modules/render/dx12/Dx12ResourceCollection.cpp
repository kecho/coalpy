#pragma once

#include <Config.h>

#if ENABLE_DX12

#include "Dx12ResourceCollection.h"
#include "Dx12Device.h"
#include "WorkBundleDb.h"
#include <coalpy.core/Assert.h>
#include <vector>
#include <sstream>

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
    if (!c->resource->config().recreatable)
        return TextureResult { ResourceResult::InvalidHandle, Texture(), "Texture is not tagged as recreatable." };

    std::vector<ResourceTable> parentTables;
    getParentTablesUnsafe(handle, parentTables);

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
    m_workDb.registerResource(handle, desc.memFlags, c->resource->defaultGpuState());

    for (auto t : parentTables)
        recreateUnsafe(t);

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

SamplerResult Dx12ResourceCollection::createSampler(const SamplerDesc& desc)
{
    std::unique_lock lock(m_resourceMutex);
    ResourceHandle resHandle;
    SmartPtr<Dx12Sampler> samplerObj = new Dx12Sampler(m_device, desc);
    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();

    outPtr->type = ResType::Sampler;
    outPtr->sampler = samplerObj;
    outPtr->handle = resHandle;

    return SamplerResult { ResourceResult::Ok, Sampler { resHandle.handleId} };
}

bool Dx12ResourceCollection::convertTableDescToResourceList(
    const ResourceTableDesc& desc,
    bool isUav,
    std::vector<Dx12Resource*>& resources,
    std::set<ResourceContainer*>& trackedContainers,
    Dx12ResourceTableResult& outResult)
{
    if (desc.resourcesCount == 0)
    {
        outResult = Dx12ResourceTableResult { ResourceResult::InvalidParameter, ResourceTable(), "Table must contain at least 1 resource." };
        return false;
    }

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
            std::stringstream ss;
            ss << "Tried to create resource table " << desc.name << " but memflags are incorrect." << (isUav ? "Resource must contain GpuWrite flag." : "Resource must contain GpuRead flag." );
            outResult = Dx12ResourceTableResult { ResourceResult::InvalidParameter, ResourceTable(), ss.str() };
            return false;
        }

        if ((container->resource->specialFlags() & ResourceSpecialFlag_TrackTables) != 0)
            trackedContainers.insert(&(*container));

        resources.push_back(&(*container->resource));
    }

    return true;
}

bool Dx12ResourceCollection::convertTableDescToSamplerList(
    const ResourceTableDesc& desc,
    std::vector<Dx12Sampler*>& samplers,
    Dx12ResourceTableResult& outResult)
{
    if (desc.resourcesCount == 0)
    {
        outResult = Dx12ResourceTableResult { ResourceResult::InvalidParameter, ResourceTable(), "Table must contain at least 1 resource." };
        return false;
    }

    for (int i = 0; i < desc.resourcesCount; ++i)
    {
        ResourceHandle h = desc.resources[i];
        bool foundResource = m_resources.contains(h);
        CPY_ASSERT(foundResource);
        if (!foundResource)
            return false;
        
        SmartPtr<ResourceContainer>& container = m_resources[h];
        if (container->type != ResType::Sampler)
        {
            outResult = Dx12ResourceTableResult { ResourceResult::InvalidParameter, ResourceTable(), "Sampler table resource must be a sampler type." };
            return false;
        }

        CPY_ASSERT(container->sampler != nullptr);
        if (container->sampler == nullptr)
            return false;

        samplers.push_back(&(*container->sampler));
    }
    
    return true;
}

Dx12ResourceTableResult Dx12ResourceCollection::createResourceTable(const ResourceTableDesc& desc, bool isUav)
{
    std::unique_lock lock(m_resourceMutex);
    std::vector<Dx12Resource*> gatheredResources;
    std::set<ResourceContainer*> containersToTrack;
    Dx12ResourceTableResult result = { ResourceResult::Ok, ResourceTable() };
    if (!convertTableDescToResourceList(desc, isUav,  gatheredResources, containersToTrack, result))
        return result;

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

    return Dx12ResourceTableResult { ResourceResult::Ok, handle };
}

InResourceTableResult Dx12ResourceCollection::createInResourceTable(const ResourceTableDesc& desc)
{
    Dx12ResourceTableResult result = createResourceTable(desc, false);
    if (!result.success())
        return InResourceTableResult { result.result, InResourceTable(), std::move(result.message) };

    m_workDb.registerTable(result.tableHandle, desc.name.c_str(), desc.resources, desc.resourcesCount, false);
    return InResourceTableResult { ResourceResult::Ok, InResourceTable { result.tableHandle.handleId } };
}

OutResourceTableResult Dx12ResourceCollection::createOutResourceTable(const ResourceTableDesc& desc)
{
    Dx12ResourceTableResult result = createResourceTable(desc, true);
    if (!result.success())
        return OutResourceTableResult { result.result, OutResourceTable(), std::move(result.message) };

    m_workDb.registerTable(result.tableHandle, desc.name.c_str(), desc.resources, desc.resourcesCount, true);
    return OutResourceTableResult { ResourceResult::Ok, OutResourceTable { result.tableHandle.handleId } };
}

SamplerTableResult Dx12ResourceCollection::createSamplerTable(const ResourceTableDesc& desc)
{
    Dx12ResourceTableResult result { ResourceResult::Ok };
    std::vector<Dx12Sampler*> samplers;
    if (!convertTableDescToSamplerList(desc, samplers, result))
        return SamplerTableResult { result.result, SamplerTable(), result.message };

    ResourceTable handle;
    auto& outPtr = m_resourceTables.allocate(handle);
    outPtr = new Dx12ResourceTable(m_device, samplers.data(), (int)samplers.size()); 

    return SamplerTableResult { ResourceResult::Ok, SamplerTable { handle.handleId } };
}

bool Dx12ResourceCollection::recreateUnsafe(ResourceTable handle)
{
    bool isValid = handle.valid() && m_resourceTables.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return false;

    SmartPtr<Dx12ResourceTable>& tableSlot = m_resourceTables[handle];
    auto it = m_workDb.tableInfos().find(handle);
    CPY_ASSERT(it != m_workDb.tableInfos().end());
    if (it == m_workDb.tableInfos().end())
        return false;

    const WorkTableInfo& info = it->second;
    ResourceTableDesc desc;
    desc.name = info.name;
    desc.resources = info.resources.data();
    desc.resourcesCount = (int)info.resources.size();
    
    std::vector<Dx12Resource*> gatheredResources;
    std::set<ResourceContainer*> containersToTrack;
    Dx12ResourceTableResult tableResult;
    bool result = convertTableDescToResourceList(desc, info.isUav, gatheredResources, containersToTrack, tableResult);
    CPY_ASSERT(result);
    if (!result)
        return false;

    tableSlot = nullptr;
    tableSlot = new Dx12ResourceTable(m_device, gatheredResources.data(), (int)gatheredResources.size(), info.isUav);

    return true;
}

void Dx12ResourceCollection::getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = handle.valid() && m_resources.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;

    SmartPtr<ResourceContainer>& container = m_resources[handle];
    memInfo.isBuffer = container->type == ResType::Buffer;
    if (memInfo.isBuffer)
    {
        auto& buffer = (Dx12Buffer&)(*container->resource);
        memInfo.byteSize = buffer.byteSize();
        memInfo.rowPitch = memInfo.byteSize;
        memInfo.width = memInfo.height = memInfo.depth = 0;
    }
    else
    {
        auto& texture = (Dx12Texture&)(*container->resource);
        memInfo.byteSize = texture.byteSize();
        texture.getCpuTextureSizes(0u, memInfo.rowPitch, memInfo.width, memInfo.height, memInfo.depth);
    }
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

    container->clear();
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

void Dx12ResourceCollection::getParentTablesUnsafe(ResourceHandle resource, std::vector<ResourceTable>& outTables)
{
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
