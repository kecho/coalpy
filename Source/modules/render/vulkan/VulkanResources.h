#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/Formats.h>
#include <vulkan/vulkan.h>
#include "VulkanDescriptorSetPools.h"
#include "VulkanCounterPool.h"
#include <vector>
#include <set>
#include <mutex>

namespace coalpy
{
namespace render
{

enum ResourceSpecialFlags : int
{
    ResourceSpecialFlag_None = 0,
    ResourceSpecialFlag_NoDeferDelete = 1 << 0,
    //ResourceSpecialFlag_CanDenyShaderResources = 1 << 1,
    ResourceSpecialFlag_TrackTables = 1 << 2,
    ResourceSpecialFlag_CpuReadback = 1 << 3,
    ResourceSpecialFlag_CpuUpload = 1 << 4,
    ResourceSpecialFlag_MapMemory = 1 << 5,
    ResourceSpecialFlag_EnableColorAttachment = 1 << 6,
};

class VulkanDevice;
class WorkBundleDb;
struct ResourceMemoryInfo;
enum { VulkanMaxMips = 14 };

struct VulkanResource
{
    enum class Type   
    {
        Buffer,
        Texture,
        Sampler
    };

    struct BufferData
    {
        bool ownsBuffer;
        bool isStorageBuffer;
        VkBufferView vkBufferView;
        VkBuffer vkBuffer;
        VkDeviceSize size;
    };

    struct TextureData
    {
        bool ownsImage;
        VkImage vkImage;
        VkImageView vkSrvView;
        VkImageView vkUavViews[VulkanMaxMips];
        VkImageSubresourceRange subresourceRange;
        TextureType textureType;
        Format format;
        int width;
        int height;
        int depth;
        uint32_t uavCounts;
    };

    ResourceHandle handle;
    Type type = Type::Buffer;
    union
    {
        BufferData bufferData;
        TextureData textureData;
        VkSampler sampler;
    };

    bool isBuffer() const { return type == Type::Buffer; }
    bool isTexture() const { return type == Type::Texture; }
    bool isSampler() const { return type == Type::Sampler; }
    
    VulkanCounterHandle counterHandle;
    MemFlags memFlags = {};
    ResourceSpecialFlags specialFlags = {};
    VkDeviceSize alignment = {};
    VkDeviceSize requestSize = {};
    VkDeviceSize actualSize = {};
    VkDeviceMemory memory = {};
    void* mappedMemory = {};

    std::set<ResourceTable> trackedTables;
};

struct VulkanResourceTable
{
    enum class Type
    {
        In, Out, Sampler
    };

    Type type = Type::In;
    VkDescriptorSetLayout layout;
    VulkanDescriptorTable descriptors;
    int countersBegin = 0;
    int countersEnd = 0;
    int descriptorsBegin = 0;
    int descriptorsEnd = 0;
    int descriptorsCount() const { return descriptorsEnd - descriptorsBegin; }
    int countersCount() const { return countersEnd - countersBegin; }
    std::unordered_map<ResourceHandle, int> trackedResources;
};

class VulkanResources
{
public:
    enum 
    {
        MaxResources = 4095
    };

    VulkanResources(VulkanDevice& device, WorkBundleDb& workDb);
    ~VulkanResources();

    BufferResult createBuffer(const BufferDesc& desc, VkBuffer resourceToAcquire = VK_NULL_HANDLE, ResourceSpecialFlags specialFlags = ResourceSpecialFlag_None);
    BufferResult createBuffer(const BufferDesc& desc, ResourceSpecialFlags specialFlags = ResourceSpecialFlag_None) { return createBuffer(desc, VK_NULL_HANDLE, specialFlags); }
    TextureResult createTexture(const TextureDesc& desc, VkImage resourceToAcquire = VK_NULL_HANDLE, ResourceSpecialFlags specialFlags = ResourceSpecialFlag_None);
    TextureResult recreateTexture(Texture texture, const TextureDesc& desc, VkImage resourceToAcquire = VK_NULL_HANDLE);
    SamplerResult createSampler (const SamplerDesc& config);
    InResourceTableResult createInResourceTable(const ResourceTableDesc& desc);
    OutResourceTableResult createOutResourceTable(const ResourceTableDesc& desc);
    SamplerTableResult createSamplerTable(const ResourceTableDesc& desc);
    VulkanResource& unsafeGetResource(ResourceHandle handle) { return m_container[handle]; }
    VulkanResourceTable& unsafeGetTable(ResourceTable handle) { return m_tables[handle]; }
    void getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo);

    void release(ResourceHandle handle);
    void release(ResourceTable handle);

private:
    TextureResult createTextureInternal(ResourceHandle handle, const TextureDesc& desc, VkImage resourceToAcquire, ResourceSpecialFlags specialFlags);
    void releaseResourceInternal(ResourceHandle handle, VulkanResource& resource);
    void releaseTableInternal(ResourceTable handle, VulkanResourceTable& table);
    void trackResources(const VulkanResource** resources, int count, ResourceTable table); 
    VkImageViewCreateInfo createVulkanImageViewDescTemplate(const TextureDesc& desc, unsigned int descDepth, VkImage image) const;
    bool queryResources(const ResourceHandle* handles, int counts, std::vector<const VulkanResource*>& outResources) const;
    bool createBindings(
        VulkanResourceTable::Type tableType,
        const VulkanResource** resources, int counts,
        std::vector<VkDescriptorSetLayoutBinding>& outBindings,
        int& descriptorsBegin, int& descriptorsEnd,
        int& countersBegin, int& countersEnd) const;

    ResourceTable createAndFillTable(
        VulkanResourceTable::Type tableType,
        const VulkanResource** resources,
        const int* uavTargetMips,
        VkDescriptorSetLayout layout,
        const VkDescriptorSetLayoutBinding* bindings,
        int descriptorsBegin, int descriptorsEnd, int countersBegin, int countersEnd);

    std::mutex m_mutex;
    VulkanDevice& m_device;
    WorkBundleDb& m_workDb;
    HandleContainer<ResourceHandle, VulkanResource, MaxResources> m_container;
    HandleContainer<ResourceTable, VulkanResourceTable, MaxResources> m_tables;
};

}
}
