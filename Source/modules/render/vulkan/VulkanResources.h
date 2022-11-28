#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.core/HandleContainer.h>
#include <vulkan/vulkan.h>

#include <mutex>

namespace coalpy
{
namespace render
{

class VulkanDevice;


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
    };

    struct TextureData
    {
        VkImage vkImage;
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

    BufferResult createBuffer(const BufferDesc& desc);
    TextureResult createTexture(const TextureDesc& desc);
    TextureResult recreateTexture(Texture texture, const TextureDesc& desc);
    InResourceTableResult createInResourceTable(const ResourceTableDesc& desc);

    void release(ResourceHandle handle);

private:
    std::mutex m_mutex;
    VulkanDevice& m_device;
    HandleContainer<ResourceHandle, VulkanResource, MaxResources> m_container;
    HandleContainer<ResourceTable, VulkanResourceTable, MaxResources> m_tables;
};

}
}
