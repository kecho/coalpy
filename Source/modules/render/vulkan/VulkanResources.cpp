#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "Config.h"
#include "VulkanFormats.h"
#include <coalpy.core/Assert.h>
#include <iostream>

namespace coalpy
{
namespace render
{

VulkanResources::VulkanResources(VulkanDevice& device)
: m_device(device)
{
}

VulkanResources::~VulkanResources()
{
}


BufferResult VulkanResources::createBuffer(const BufferDesc& desc)
{
    std::unique_lock lock(m_mutex);
    ResourceHandle handle;
    VulkanResource& resource = m_container.allocate(handle);
    if (!handle.valid())
        return BufferResult  { ResourceResult::InvalidHandle, Buffer(), "Not enough slots." };

    VkMemoryPropertyFlags memProFlags = 0u;
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.usage |= desc.isConstantBuffer ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : (VkBufferUsageFlags)0u;
    if (desc.type == BufferType::Standard)
    {
        if ((desc.memFlags & MemFlag_GpuRead) != 0)
            createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        if ((desc.memFlags & MemFlag_GpuWrite) != 0)
            createInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        createInfo.size = getVkFormatStride(desc.format) * desc.elementCount;
    }
    else if (desc.type == BufferType::Structured || desc.type == BufferType::Raw)
    {
        //in vulkan it doesnt matter, if structured or raw, always set the storage buffer bit.
        createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        createInfo.size = desc.stride * desc.elementCount;
    }

    memProFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //not exposed, cant do async in coalpy yet.
    createInfo.queueFamilyIndexCount = 1;
    uint32_t desfaultQueueFam = m_device.graphicsFamilyQueueIndex();
    createInfo.pQueueFamilyIndices = &desfaultQueueFam;

    auto& bufferData = resource.bufferData;
    CPY_ASSERT_MSG(!desc.isAppendConsume, "Append consume not supported by vulkan yet");
    resource.type = VulkanResource::Type::Buffer;
    resource.handle = handle;
    if (vkCreateBuffer(m_device.vkDevice(), &createInfo, nullptr, &bufferData.vkBuffer) != VK_SUCCESS)
    {
        m_container.free(handle);
        return BufferResult  { ResourceResult::InvalidParameter, Buffer(), "Failed to create buffer." };
    }

    VkMemoryRequirements memReqs = {};
    vkGetBufferMemoryRequirements(m_device.vkDevice(), bufferData.vkBuffer, &memReqs);
    resource.actualSize = memReqs.size;
    resource.alignment = memReqs.alignment;

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    if (!m_device.findMemoryType(memReqs.memoryTypeBits, 0u, allocInfo.memoryTypeIndex)) 
    {
        vkDestroyBuffer(m_device.vkDevice(), bufferData.vkBuffer, nullptr);
        m_container.free(handle);
        return BufferResult  { ResourceResult::InternalApiFailure, Buffer(), "Failed to find a correct category of memory for this buffer." };
    }

    if (vkAllocateMemory(m_device.vkDevice(), &allocInfo, nullptr, &resource.memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(m_device.vkDevice(), bufferData.vkBuffer, nullptr);
        m_container.free(handle);
        return BufferResult  { ResourceResult::InternalApiFailure, Buffer(), "Failed to allocating buffer memory." };
    }

    if (vkBindBufferMemory(m_device.vkDevice(), bufferData.vkBuffer, resource.memory, 0u) != VK_SUCCESS)
    {
        vkDestroyBuffer(m_device.vkDevice(), bufferData.vkBuffer, nullptr);
        vkFreeMemory(m_device.vkDevice(), resource.memory, nullptr);
        m_container.free(handle);
        return BufferResult  { ResourceResult::InternalApiFailure, Buffer(), "Failed to bind memory into buffer." };
    }

    bufferData.vkBufferView = {};
    if (desc.type == BufferType::Standard)
    {
        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.buffer = bufferData.vkBuffer;
        bufferViewInfo.format = getVkFormat(desc.format);
        bufferViewInfo.offset = 0u;
        bufferViewInfo.range = createInfo.size;
        if (vkCreateBufferView(m_device.vkDevice(), &bufferViewInfo, nullptr, &bufferData.vkBufferView) != VK_SUCCESS)
        {
            vkDestroyBuffer(m_device.vkDevice(), bufferData.vkBuffer, nullptr);
            vkFreeMemory(m_device.vkDevice(), resource.memory, nullptr);
            m_container.free(handle);
            return BufferResult  { ResourceResult::InvalidParameter, Buffer(), "Failed to create buffer view for standard buffer." };
        }
    }


    return BufferResult { ResourceResult::Ok, { handle.handleId } };
}

TextureResult VulkanResources::createTexture(const TextureDesc& desc)
{
    std::unique_lock lock(m_mutex);
    ResourceHandle handle;
    VulkanResource& resource = m_container.allocate(handle);
    if (!handle.valid())
        return TextureResult  { ResourceResult::InvalidHandle, Texture(), "Not enough slots." };

    VkMemoryPropertyFlags memProFlags = 0u;
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    if ((desc.memFlags & MemFlag_GpuWrite) != 0)
        createInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    else
        createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    //TODO: figure out how to do cube maps
    bool isArray = (desc.type == TextureType::k2dArray || desc.type == TextureType::CubeMapArray);
    if (desc.type == TextureType::k2d || desc.type == TextureType::k2dArray || desc.type == TextureType::CubeMap || desc.type == TextureType::CubeMapArray)
        createInfo.imageType = VK_IMAGE_TYPE_2D;
    else if (desc.type == TextureType::k3d)
        createInfo.imageType = VK_IMAGE_TYPE_3D;

    createInfo.format = getVkFormat(desc.format);
    createInfo.extent = VkExtent3D { desc.width, desc.height, desc.type == TextureType::k3d ? desc.depth : 1 };
    createInfo.mipLevels = desc.mipLevels;
    createInfo.arrayLayers = isArray ? desc.depth : 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //not exposed, cant do async in coalpy yet.
    createInfo.queueFamilyIndexCount = 1;
    uint32_t desfaultQueueFam = m_device.graphicsFamilyQueueIndex();
    createInfo.pQueueFamilyIndices = &desfaultQueueFam;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    auto& textureData = resource.textureData;
    resource.type = VulkanResource::Type::Texture;
    resource.handle = handle;

    if (vkCreateImage(m_device.vkDevice(), &createInfo, nullptr, &textureData.vkImage) != VK_SUCCESS)
    {
        m_container.free(handle);
        std::cout << "Fail" << std::endl;
        return TextureResult  { ResourceResult::InvalidParameter, Texture(), "Failed to create texture." };
    }

    VkMemoryRequirements memReqs = {};
    vkGetImageMemoryRequirements(m_device.vkDevice(), textureData.vkImage, &memReqs);
    resource.actualSize = memReqs.size;
    resource.alignment = memReqs.alignment;

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    if (!m_device.findMemoryType(memReqs.memoryTypeBits, 0u, allocInfo.memoryTypeIndex)) 
    {
        vkDestroyImage(m_device.vkDevice(), textureData.vkImage, nullptr);
        m_container.free(handle);
        return TextureResult { ResourceResult::InternalApiFailure, Texture(), "Failed to find a correct category of memory for this texture." };
    }

    if (vkAllocateMemory(m_device.vkDevice(), &allocInfo, nullptr, &resource.memory) != VK_SUCCESS)
    {
        vkDestroyImage(m_device.vkDevice(), textureData.vkImage, nullptr);
        m_container.free(handle);
        return TextureResult { ResourceResult::InternalApiFailure, Texture(), "Failed to allocating buffer memory." };
    }

    if (vkBindImageMemory(m_device.vkDevice(), textureData.vkImage, resource.memory, 0u) != VK_SUCCESS)
    {
        vkDestroyImage(m_device.vkDevice(), textureData.vkImage, nullptr);
        vkFreeMemory(m_device.vkDevice(), resource.memory, nullptr);
        m_container.free(handle);
        return TextureResult  { ResourceResult::InternalApiFailure, Texture(), "Failed to bind memory into vkimage." };
    }

    return TextureResult { ResourceResult::Ok, { handle.handleId } };
}

TextureResult VulkanResources::recreateTexture(Texture texture, const TextureDesc& desc)
{
    std::unique_lock lock(m_mutex);
    return TextureResult();
}

void VulkanResources::release(ResourceHandle handle)
{
    CPY_ASSERT(handle.valid());
    if (!handle.valid())
        return;

    CPY_ASSERT(m_container.contains(handle));
    if (!m_container.contains(handle))
        return;

    std::unique_lock lock(m_mutex);
    VulkanResource& resource = m_container[handle];

    if (resource.isBuffer())
    {
        if (resource.bufferData.vkBufferView)
            vkDestroyBufferView(m_device.vkDevice(), resource.bufferData.vkBufferView, nullptr);
        vkDestroyBuffer(m_device.vkDevice(), resource.bufferData.vkBuffer, nullptr);
    }
    else if (resource.isTexture())
    {
        if (resource.textureData.vkImage)
            vkDestroyImage(m_device.vkDevice(), resource.textureData.vkImage, nullptr);
    }
    vkFreeMemory(m_device.vkDevice(), resource.memory, nullptr);

    m_container.free(handle);
}

}
}
