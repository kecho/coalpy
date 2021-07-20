#pragma once

#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include "Dx12Resources.h"
#include <mutex>
#include <vector>
#include <set>
#include <unordered_map>

struct ID3D12Resource;

namespace coalpy
{
namespace render
{

class WorkBundleDb;
class Dx12Device;

class Dx12ResourceCollection
{
public:
    Dx12ResourceCollection(Dx12Device& device, WorkBundleDb& workDb);
    ~Dx12ResourceCollection();

    Texture createTexture(const TextureDesc& desc, ID3D12Resource* resourceToAcquire = nullptr, ResourceSpecialFlags flags = ResourceSpecialFlag_None);
    Buffer  createBuffer (const BufferDesc& desc, ID3D12Resource* resourceToAcquire = nullptr, ResourceSpecialFlags flags = ResourceSpecialFlag_None);
    InResourceTable  createInResourceTable(const ResourceTableDesc& desc);
    OutResourceTable createOutResourceTable(const ResourceTableDesc& desc);

    void release(ResourceHandle resource);
    void release(ResourceTable resource);

    Dx12ResourceTable& unsafeGetTable(ResourceTable handle) { return *(m_resourceTables[handle]); }
    Dx12Resource& unsafeGetResource(ResourceHandle handle) { return *(m_resources[handle]->resource); }

    void getParentTables(ResourceHandle resource, std::vector<ResourceTable>& outTables);
    void recreate(ResourceTable resource);
    void recreateTexture(Texture handle, const TextureDesc& desc, ID3D12Resource* resourceToAcquire = nullptr);

private:


    ResourceTable createResourceTable(const ResourceTableDesc& desc, bool isUav);
    enum class ResType { Texture, Buffer };
    class ResourceContainer : public RefCounted
    {
    public:
        ResType type = ResType::Texture;
        ResourceHandle handle;
        SmartPtr<Dx12Resource> resource;
        std::set<ResourceTable> parentTables;
    };

    bool convertTableDescToResourceList(
        const ResourceTableDesc& desc,
        std::vector<Dx12Resource*>& resources,
        std::set<ResourceContainer*>& trackedContainers,
        bool isUav);

    std::unordered_map<ResourceTable, std::set<ResourceHandle>> m_trackedTableToResources;

    Dx12Device& m_device;
    WorkBundleDb& m_workDb;
    std::mutex m_resourceMutex;
    HandleContainer<ResourceHandle, SmartPtr<ResourceContainer>> m_resources;
    HandleContainer<ResourceTable , SmartPtr<Dx12ResourceTable>> m_resourceTables;
};

}
}
