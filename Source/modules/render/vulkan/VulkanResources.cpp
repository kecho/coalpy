#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "Config.h"
#include "VulkanFormats.h"
#include <coalpy.core/Assert.h>
#include <variant>
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
    resource.bufferData.isStorageBuffer = false;
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
        resource.bufferData.isStorageBuffer = true;
    }
    resource.memFlags = desc.memFlags;

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
    bufferData.size = createInfo.size;
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

VkImageViewCreateInfo VulkanResources::createVulkanImageViewDescTemplate(const TextureDesc& desc, VkImage image) const
{
    const bool isArray = (desc.type == TextureType::k2dArray || desc.type == TextureType::CubeMapArray);
    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr };
    viewInfo.flags = 0u;
    viewInfo.image = image;
    
    switch (desc.type)
    {
    case TextureType::k1d:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D; break;
    case TextureType::k2dArray:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
    case TextureType::k3d:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D; break;
    case TextureType::CubeMap:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; break;
    case TextureType::CubeMapArray:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
    case TextureType::k2d:
    default:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; break;
    }
    
    viewInfo.format = getVkFormat(desc.format);
    viewInfo.components = {};
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, desc.mipLevels, 0u, isArray ? desc.depth : 1 };
    return viewInfo;
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
    
    resource.memFlags = desc.memFlags;

    if ((desc.memFlags & MemFlag_GpuRead) != 0)
        createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    if ((desc.memFlags & MemFlag_GpuWrite) != 0)
        createInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    bool isArray = (desc.type == TextureType::k2dArray || desc.type == TextureType::CubeMapArray);
    switch (desc.type)
    {
    case TextureType::k1d:
        createInfo.imageType = VK_IMAGE_TYPE_1D; break;
    case TextureType::k3d:
        createInfo.imageType = VK_IMAGE_TYPE_3D; break;
    case TextureType::k2d:
    case TextureType::k2dArray:
    case TextureType::CubeMap:
    case TextureType::CubeMapArray:
    default:
        createInfo.imageType = VK_IMAGE_TYPE_2D; break;
    }

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

    if ((resource.memFlags & MemFlag_GpuRead) != 0)
    {
        VkImageViewCreateInfo srvViewInfo = createVulkanImageViewDescTemplate(desc, textureData.vkImage);
        if (vkCreateImageView(m_device.vkDevice(), &srvViewInfo, nullptr, &textureData.vkSrvView) != VK_SUCCESS)
        {
            vkDestroyImage(m_device.vkDevice(), textureData.vkImage, nullptr);
            vkFreeMemory(m_device.vkDevice(), resource.memory, nullptr);
            m_container.free(handle);
            return TextureResult { ResourceResult::InternalApiFailure, Texture(), "Failed to create a texture image view" };
        }
    }

    textureData.uavCounts = 0;
    if ((resource.memFlags & MemFlag_GpuWrite) != 0)
    {
        VkImageViewCreateInfo uavViewInfo = createVulkanImageViewDescTemplate(desc, textureData.vkImage);
        for (int mip = 0; mip < std::min(desc.mipLevels, (unsigned)VulkanMaxMips); ++mip)
        {
            uavViewInfo.subresourceRange.baseMipLevel = mip;
            uavViewInfo.subresourceRange.levelCount = 1;
            if (vkCreateImageView(m_device.vkDevice(), &uavViewInfo, nullptr, &textureData.vkUavViews[mip]) != VK_SUCCESS)
            {
                CPY_ERROR_MSG(false, "Failed creating mip UAV for texture.");
            }
            ++textureData.uavCounts;
        }
    }

    return TextureResult { ResourceResult::Ok, { handle.handleId } };
}

TextureResult VulkanResources::recreateTexture(Texture texture, const TextureDesc& desc)
{
    std::unique_lock lock(m_mutex);
    return TextureResult();
}

bool VulkanResources::queryResources(const ResourceHandle* handles, int counts, std::vector<const VulkanResource*>& outResources) const
{
    outResources.reserve(counts);
    for (int i = 0; i < counts; ++i)
    {
        if (!m_container.contains(handles[i]))
        {
            return false;
        }
        
        const VulkanResource& resource = m_container[handles[i]];
        outResources.push_back(&resource);
    }

    return true;
}

bool VulkanResources::createBindings(VulkanResourceTable::Type tableType, const VulkanResource** resources, int counts, std::vector<VkDescriptorSetLayoutBinding>& outBindings) const
{
    const bool isRead = tableType == VulkanResourceTable::Type::In;
    auto flagToCheck = isRead ? MemFlag_GpuRead : MemFlag_GpuWrite;
    outBindings.reserve(counts);
    for (int i = 0; i < counts; ++i)
    {
        const VulkanResource& resource = *resources[i];
        if ((resource.memFlags & flagToCheck) == 0)
            return false;

        outBindings.emplace_back();
        VkDescriptorSetLayoutBinding& binding = outBindings.back();
        binding = {};
        binding.binding = (uint32_t)i;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binding.descriptorCount = 1;
        if (resource.isBuffer())
            binding.descriptorType = resource.bufferData.isStorageBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : (isRead ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
        else if (resource.isTexture())
            binding.descriptorType = isRead ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }

    return true;
}

ResourceTable VulkanResources::createAndFillTable(
        VulkanResourceTable::Type tableType,
        const VulkanResource** resources,
        const VkDescriptorSetLayoutBinding* bindings,
        const int* uavTargetMips,
        int counts, VkDescriptorSetLayout layout)
{
    bool isRead = tableType == VulkanResourceTable::Type::In;

    union DescriptorVariantInfo
    {
        VkDescriptorImageInfo imageInfo;
        VkDescriptorBufferInfo texelBufferInfo;
        VkBufferView bufferInfo;
    };

    ResourceTable handle;
    VulkanResourceTable& table = m_tables.allocate(handle);
    table.type = tableType;
    table.layout = layout;
    table.descriptors = m_device.descriptorSetPools().allocate(layout);

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<DescriptorVariantInfo> variantInfos;
    writes.reserve(counts);
    variantInfos.reserve(counts);

    for (int i = 0; i < (int)counts; ++i)
    {
        const VulkanResource& resource = *resources[i];
        writes.emplace_back();
        VkWriteDescriptorSet& write = writes.back();
        write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr };
        write.dstSet = table.descriptors.descriptors;
        write.dstBinding = i;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = bindings[i].descriptorType;
        
        variantInfos.emplace_back();
        DescriptorVariantInfo& variantInfo = variantInfos.back();
        if (resource.isBuffer())
        {
            if (resource.bufferData.isStorageBuffer)
            {
                auto& info = variantInfo.texelBufferInfo;
                info.buffer = resource.bufferData.vkBuffer;
                info.offset = 0u;
                info.range = resource.bufferData.size;
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                write.pBufferInfo = &info;
            }
            else
            {
                auto& info = variantInfo.bufferInfo;
                info = resource.bufferData.vkBufferView;
                write.descriptorType = isRead ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                write.pTexelBufferView = &info;
            }
        }
        else if (resource.isTexture())
        {
            auto& info = variantInfo.imageInfo;
            info = {};
            if (isRead)
                info.imageView = resource.textureData.vkSrvView;
            else
                info.imageView = uavTargetMips ? resource.textureData.vkUavViews[uavTargetMips[i]] : resource.textureData.vkUavViews[0];
            info.imageLayout = isRead ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
            write.descriptorType = isRead ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo = &info;
        }
    }

    vkUpdateDescriptorSets(m_device.vkDevice(), (uint32_t)writes.size(), writes.data(), 0u, nullptr);
    
    return handle;
}

InResourceTableResult VulkanResources::createInResourceTable(const ResourceTableDesc& desc)
{
    std::unique_lock lock(m_mutex);
    std::vector<const VulkanResource*> resources;
    if (!queryResources(desc.resources, desc.resourcesCount, resources))
            return InResourceTableResult { ResourceResult::InvalidHandle, InResourceTable(), "Passed an invalid resource to in resource table" };

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    if (!createBindings(VulkanResourceTable::Type::In, resources.data(), (int)resources.size(), bindings))
            return InResourceTableResult { ResourceResult::InvalidParameter, InResourceTable(), "All resources in InTable must have the flag GpuRead." };

    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr };
    layoutInfo.bindingCount = (uint32_t)bindings.size();
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout = {};
    if (vkCreateDescriptorSetLayout(m_device.vkDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        return InResourceTableResult { ResourceResult::InvalidParameter, InResourceTable(), "Could not create descriptor set layout." };
    }

    ResourceTable handle = createAndFillTable(VulkanResourceTable::Type::In, resources.data(), bindings.data(), desc.uavTargetMips, (int)resources.size(), layout);
    return InResourceTableResult { ResourceResult::Ok, InResourceTable { handle.handleId } };
}

OutResourceTableResult VulkanResources::createOutResourceTable(const ResourceTableDesc& desc)
{
    std::unique_lock lock(m_mutex);
    std::vector<const VulkanResource*> resources;
    if (!queryResources(desc.resources, desc.resourcesCount, resources))
            return OutResourceTableResult { ResourceResult::InvalidHandle, OutResourceTable(), "Passed an invalid resource to out resource table" };

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    if (!createBindings(VulkanResourceTable::Type::Out, resources.data(), (int)resources.size(), bindings))
            return OutResourceTableResult { ResourceResult::InvalidParameter, OutResourceTable(), "All resources in InTable must have the flag GpuWrite." };

    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr };
    layoutInfo.bindingCount = (uint32_t)bindings.size();
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout = {};
    if (vkCreateDescriptorSetLayout(m_device.vkDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        return OutResourceTableResult { ResourceResult::InvalidParameter, OutResourceTable(), "Could not create descriptor set layout." };
    }

    ResourceTable handle = createAndFillTable(VulkanResourceTable::Type::Out, resources.data(), bindings.data(), desc.uavTargetMips, (int)resources.size(), layout);
    return OutResourceTableResult { ResourceResult::Ok, OutResourceTable { handle.handleId } };
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
        if (resource.textureData.vkSrvView)
            vkDestroyImageView(m_device.vkDevice(), resource.textureData.vkSrvView, nullptr);
        for (int mip = 0; mip < resource.textureData.uavCounts; ++mip)
            vkDestroyImageView(m_device.vkDevice(), resource.textureData.vkUavViews[mip], nullptr);
        if (resource.textureData.vkImage)
            vkDestroyImage(m_device.vkDevice(), resource.textureData.vkImage, nullptr);
    }

    vkFreeMemory(m_device.vkDevice(), resource.memory, nullptr);

    m_container.free(handle);
}

void VulkanResources::release(ResourceTable handle)
{
    CPY_ASSERT(handle.valid());
    if (!handle.valid())
        return;

    CPY_ASSERT(m_tables.contains(handle));
    if (!m_tables.contains(handle))
        return;

    std::unique_lock lock(m_mutex);
    VulkanResourceTable& table = m_tables[handle];
    vkDestroyDescriptorSetLayout(m_device.vkDevice(), table.layout, nullptr);
    m_device.descriptorSetPools().free(table.descriptors);
    m_tables.free(handle);
}

}
}
