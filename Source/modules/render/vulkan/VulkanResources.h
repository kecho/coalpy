#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.core/HandleContainer.h>
#include <vulkan/vulkan.h>
#include "VulkanDescriptorSetPools.h"
#include <vector>
#include <mutex>

namespace coalpy
{
namespace render
{

enum ResourceSpecialFlags : int
{
    ResourceSpecialFlag_None = 0,
    //unused:
    //ResourceSpecialFlag_NoDeferDelete = 1 << 0,
    //ResourceSpecialFlag_CanDenyShaderResources = 1 << 1,
    //ResourceSpecialFlag_TrackTables = 1 << 2,
    ResourceSpecialFlag_CpuReadback = 1 << 3,
    ResourceSpecialFlag_CpuUpload = 1 << 4,
};

class VulkanDevice;
enum { VulkanMaxMips = 14 };

struct VulkanResource
{
    enum class Type   
    {
        Buffer,
        Texture
    };

    struct BufferData
    {
        bool isStorageBuffer;
        VkBufferView vkBufferView;
        VkBuffer vkBuffer;
        VkDeviceSize size;
    };

    struct TextureData
    {
        VkImage vkImage;
        VkImageView vkSrvView;
        VkImageView vkUavViews[VulkanMaxMips];
        uint32_t uavCounts;
    };

    ResourceHandle handle;
    Type type = Type::Buffer;
    union
    {
        BufferData bufferData;
        TextureData textureData;
    };

    bool isBuffer() const { return type == Type::Buffer; }
    bool isTexture() const { return type == Type::Texture; }
    
    MemFlags memFlags;
    VkDeviceSize alignment;
    VkDeviceSize actualSize;
    VkDeviceMemory memory;
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
};

class VulkanResources
{
public:
    enum 
    {
        MaxResources = 4095
    };

    VulkanResources(VulkanDevice& device);
    ~VulkanResources();

    BufferResult createBuffer(const BufferDesc& desc, ResourceSpecialFlags specialFlags = ResourceSpecialFlag_None);
    TextureResult createTexture(const TextureDesc& desc);
    TextureResult recreateTexture(Texture texture, const TextureDesc& desc);
    InResourceTableResult createInResourceTable(const ResourceTableDesc& desc);
    OutResourceTableResult createOutResourceTable(const ResourceTableDesc& desc);
    VulkanResource& unsafeGetResource(ResourceHandle handle) { return m_container[handle]; }

    void release(ResourceHandle handle);
    void release(ResourceTable handle);

private:
    VkImageViewCreateInfo createVulkanImageViewDescTemplate(const TextureDesc& desc, VkImage image) const;
    bool queryResources(const ResourceHandle* handles, int counts, std::vector<const VulkanResource*>& outResources) const;
    bool createBindings(VulkanResourceTable::Type tableType, const VulkanResource** resources, int counts, std::vector<VkDescriptorSetLayoutBinding>& outBindings) const;

    ResourceTable createAndFillTable(
        VulkanResourceTable::Type tableType,
        const VulkanResource** resources,
        const VkDescriptorSetLayoutBinding* bindings,
        const int* uavTargetMips,
        int counts, VkDescriptorSetLayout layout);

    std::mutex m_mutex;
    VulkanDevice& m_device;
    HandleContainer<ResourceHandle, VulkanResource, MaxResources> m_container;
    HandleContainer<ResourceTable, VulkanResourceTable, MaxResources> m_tables;
};

}
}
