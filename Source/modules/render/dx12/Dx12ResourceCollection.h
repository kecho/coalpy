#pragma once

#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include "Dx12Resources.h"
#include <mutex>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>

struct ID3D12Resource;

namespace coalpy
{
namespace render
{

class WorkBundleDb;
class Dx12Device;

struct Dx12ResourceTableResult
{
    bool success() const { return result == ResourceResult::Ok; }
    ResourceResult result;
    ResourceTable tableHandle;
    std::string message;
};

class Dx12ResourceCollection
{
public:
    Dx12ResourceCollection(Dx12Device& device, WorkBundleDb& workDb);
    ~Dx12ResourceCollection();

    TextureResult createTexture(const TextureDesc& desc, ID3D12Resource* resourceToAcquire = nullptr, ResourceSpecialFlags flags = ResourceSpecialFlag_None);
    BufferResult  createBuffer (const BufferDesc& desc, ID3D12Resource* resourceToAcquire = nullptr, ResourceSpecialFlags flags = ResourceSpecialFlag_None);
    SamplerResult  createSampler (const SamplerDesc& desc);
    InResourceTableResult createInResourceTable(const ResourceTableDesc& desc);
    OutResourceTableResult createOutResourceTable(const ResourceTableDesc& desc);
    SamplerTableResult createSamplerTable(const ResourceTableDesc& desc);

    void getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo);

    void release(ResourceHandle resource);
    void release(ResourceTable resource);

    void lock() { m_resourceMutex.lock(); }
    Dx12ResourceTable& unsafeGetTable(ResourceTable handle) { return *(m_resourceTables[handle]); }
    Dx12Resource& unsafeGetResource(ResourceHandle handle) { return *(m_resources[handle]->resource); }
    void unlock() { m_resourceMutex.unlock(); }

    bool recreateUnsafe(ResourceTable resource);
    TextureResult recreateTexture(Texture handle, const TextureDesc& desc, ID3D12Resource* resourceToAcquire = nullptr);

    void getParentTablesUnsafe(ResourceHandle resource, std::vector<ResourceTable>& outTables);

private:
    Dx12ResourceTableResult createResourceTable(const ResourceTableDesc& desc, bool isUav);
    enum class ResType { Texture, Buffer, Sampler };
    class ResourceContainer : public RefCounted
    {
    public:
        ResType type = ResType::Texture;
        ResourceHandle handle;
        SmartPtr<Dx12Resource> resource;
        SmartPtr<Dx12Sampler> sampler;
        std::set<ResourceTable> parentTables;

        void clear()
        {
            handle = ResourceHandle();
            resource = nullptr;
            sampler = nullptr;
            parentTables.clear();
        }
    };

    bool convertTableDescToResourceList(
        const ResourceTableDesc& desc,
        bool isUav,
        std::vector<Dx12Resource*>& resources,
        std::set<ResourceContainer*>& trackedContainers,
        Dx12ResourceTableResult& outResult);

    std::unordered_map<ResourceTable, std::set<ResourceHandle>> m_trackedTableToResources;

    Dx12Device& m_device;
    WorkBundleDb& m_workDb;
    std::mutex m_resourceMutex;
    HandleContainer<ResourceHandle, SmartPtr<ResourceContainer>, 256> m_resources;
    HandleContainer<ResourceTable , SmartPtr<Dx12ResourceTable>, 256> m_resourceTables;
};

}
}
