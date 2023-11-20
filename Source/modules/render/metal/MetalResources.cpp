#include "Config.h"
#if ENABLE_METAL

#include <Metal/Metal.h>
#include "MetalResources.h"
#include "MetalDevice.h"
#include "WorkBundleDb.h"

namespace coalpy
{
namespace render
{

const int g_typeTranslations[(int)TextureType::Count] =
{
    MTLTextureType1D,       // TextureType::k1d
    MTLTextureType2D,       // TextureType::k2d
    MTLTextureType2DArray,  // TextureType::k2dArray
    MTLTextureType3D,       // TextureType::k3d
    MTLTextureTypeCube,     // TextureType::CubeMap
    MTLTextureTypeCubeArray // TextureType::CubeMapArray
};

const int g_formatTranslations[(int)Format::MAX_COUNT] =
{
    MTLPixelFormatRGBA32Float,     // RGBA_32_FLOAT,
    MTLPixelFormatRGBA32Uint,      // RGBA_32_UINT,
    MTLPixelFormatRGBA32Sint,      // RGBA_32_SINT,
    -1,                            // RGBA_32_TYPELESS,
    -1,                            // RGB_32_FLOAT,
    -1,                            // RGB_32_UINT,
    -1,                            // RGB_32_SINT,
    -1,                            // RGB_32_TYPELESS,
    MTLPixelFormatRG32Float,       // RG_32_FLOAT,
    MTLPixelFormatRG32Uint,        // RG_32_UINT,
    MTLPixelFormatRG32Sint,        // RG_32_SINT,
    -1,                            // RG_32_TYPELESS,
    MTLPixelFormatRGBA16Float,     // RGBA_16_FLOAT,
    MTLPixelFormatRGBA16Uint,      // RGBA_16_UINT,
    MTLPixelFormatRGBA16Sint,      // RGBA_16_SINT,
    MTLPixelFormatRGBA16Unorm,     // RGBA_16_UNORM,
    MTLPixelFormatRGBA16Snorm,     // RGBA_16_SNORM,
    -1,                            // RGBA_16_TYPELESS,
    MTLPixelFormatRGBA8Uint,       // RGBA_8_UINT,
    MTLPixelFormatRGBA8Sint,       // RGBA_8_SINT,
    MTLPixelFormatRGBA8Unorm,      // RGBA_8_UNORM,
    MTLPixelFormatBGRA8Unorm,      // BGRA_8_UNORM,
    MTLPixelFormatRGBA8Unorm_sRGB, // RGBA_8_UNORM_SRGB,
    MTLPixelFormatBGRA8Unorm_sRGB, // BGRA_8_UNORM_SRGB,
    MTLPixelFormatRGBA8Snorm,      // RGBA_8_SNORM,
    -1,                            // RGBA_8_TYPELESS,
    MTLPixelFormatDepth32Float,    // D32_FLOAT,
    MTLPixelFormatR32Float,        // R32_FLOAT,
    MTLPixelFormatR32Uint,         // R32_UINT,
    MTLPixelFormatR32Sint,         // R32_SINT,
    -1,                            // R32_TYPELESS,
    MTLPixelFormatDepth16Unorm,    // D16_FLOAT,
    MTLPixelFormatR16Float,        // R16_FLOAT,
    MTLPixelFormatR16Uint,         // R16_UINT,
    MTLPixelFormatR16Sint,         // R16_SINT,
    MTLPixelFormatR16Unorm,        // R16_UNORM,
    MTLPixelFormatR16Snorm,        // R16_SNORM,
    -1,                            // R16_TYPELESS,
    MTLPixelFormatRG16Float,       // RG16_FLOAT,
    MTLPixelFormatRG16Uint,        // RG16_UINT,
    MTLPixelFormatRG16Sint,        // RG16_SINT,
    MTLPixelFormatRG16Unorm,       // RG16_UNORM,
    MTLPixelFormatRG16Snorm,       // RG16_SNORM,
    -1,                            // RG16_TYPELESS,
    MTLPixelFormatR8Unorm,         // R8_UNORM
    MTLPixelFormatR8Sint,          // R8_SINT
    MTLPixelFormatR8Uint,          // R8_UINT
    MTLPixelFormatR8Snorm,         // R8_SNORM
    -1                             // R8_TYPELESS
};

static bool queryResources(
    const ResourceHandle* handles,
    int counts,
    const HandleContainer<ResourceHandle, MetalResource, MetalResources::MaxResources>& container,
    std::vector<const MetalResource*>& outResources
)
{
    outResources.reserve(counts);
    for (int i = 0; i < counts; ++i)
    {
        if (!container.contains(handles[i]))
        {
            return false;
        }
        
        const MetalResource& resource = container[handles[i]];
        outResources.push_back(&resource);
    }

    return true;
}

static ResourceTable createAndFillTable(
    MetalResourceTable::Type tableType,
    const MetalResource** resources,
    const int* uavTargetMips,
    HandleContainer<ResourceTable, MetalResourceTable, MetalResources::MaxResources>* tables
)
{
    ResourceTable handle;
    MetalResourceTable& table = tables->allocate(handle);
    table.type = tableType;

    // TODO (Apoorva): Do stuff relevant to the Metal API here
    
    return handle;
}

static void trackResources(
    const MetalResource** resources,
    int count,
    ResourceTable table,
    HandleContainer<ResourceTable, MetalResourceTable, MetalResources::MaxResources>* tables
)
{
    MetalResourceTable& tableContainer = (*tables)[table];
    for (int i = 0; i < count; ++i)
    {
        const MetalResource& resource = *resources[i];
        if ((resource.specialFlags & ResourceSpecialFlag_TrackTables) == 0)
            continue;

        std::set<ResourceTable>& trackedTables = const_cast<std::set<ResourceTable>&>(resource.trackedTables);
        trackedTables.insert(table);
        tableContainer.trackedResources[resource.handle] = i;
    }
}

static void releaseResourceInternal(
    ResourceHandle handle,
    MetalResource& resource,
    HandleContainer<ResourceTable, MetalResourceTable, MetalResources::MaxResources>* tables,
    WorkBundleDb* workDb
)
{
    for (ResourceTable table : resource.trackedTables)
    {
        MetalResourceTable& tableContainer = (*tables)[table];
        tableContainer.trackedResources.erase(handle);
    }
    resource.trackedTables.clear();

    if (resource.type == MetalResource::Type::Buffer)
    {
        if ((resource.specialFlags & ResourceSpecialFlag_NoDeferDelete) == 0)
        {
            // TODO (Apoorva): Implement GC for Metal
            // m_device.gc().deferRelease(
            //     resource.bufferData.ownsBuffer ? resource.bufferData.vkBuffer : VK_NULL_HANDLE,
            //     resource.bufferData.vkBufferView,
            //     resource.bufferData.ownsBuffer ? resource.memory : VK_NULL_HANDLE,
            //     resource.counterHandle);
        }
        else
        {
            resource.bufferData.mtlBuffer = nil;
        }
        workDb->unregisterResource(handle);
    }
    else if (resource.type == MetalResource::Type::Texture)
    {
        if ((resource.specialFlags & ResourceSpecialFlag_NoDeferDelete) == 0)
        {
            // TODO (Apoorva): Implement GC for Metal
            // m_device.gc().deferRelease(
            //     resource.textureData.ownsImage ? resource.textureData.vkImage : VK_NULL_HANDLE,
            //     resource.textureData.vkUavViews, resource.textureData.uavCounts,
            //     resource.textureData.vkSrvView,
            //     resource.textureData.ownsImage ? resource.memory : VK_NULL_HANDLE);
        }
        else
        {
            resource.textureData.mtlTexture = nil;
        }
        workDb->unregisterResource(handle);
    }
    else if (resource.type == MetalResource::Type::Sampler)
    {
        // TODO (Apoorva): Release the sampler, if that's a thing in Metal
    }
}

MetalResources::MetalResources(MetalDevice& device, WorkBundleDb& workDb) :
    m_device(device),
    m_workDb(workDb)
{
}

MetalResources::~MetalResources()
{
    // TODO (Apoorva): Release resources and tables
    // m_container.forEach([this](ResourceHandle handle, MetalResource& resource)
    // {
    //     releaseResourceInternal(handle, resource);
    // });

    // m_tables.forEach([this](ResourceTable handle, MetalResourceTable& table)
    // {
    //     releaseTableInternal(handle, table);
    // });
}

BufferResult MetalResources::createBuffer(
    const BufferDesc& desc,
    ResourceSpecialFlags specialFlags)
{
    std::unique_lock lock(m_mutex);
    ResourceHandle handle;
    MetalResource& resource = m_container.allocate(handle);
    if (!handle.valid())
        return BufferResult  { ResourceResult::InvalidHandle, Buffer(), "Not enough slots." };

    resource.type = MetalResource::Type::Buffer;
    // TODO (Apoorva): Check if certain desc.memflags are incompatible with
    // certain special flags. Check VulkanResources.cpp::createBuffer() or
    // Dx12ResourceCollection.cpp::createBuffer() for examples.

    // TODO (Apoorva): Get the stride in bytes for the given texture format in desc.format
    int stride = 4 * 1; // 4 channels, 1 byte per channel
    int sizeInBytes = stride * desc.elementCount;

    MTLResourceOptions options = MTLResourceStorageModeShared;
    resource.bufferData.mtlBuffer = [m_device.mtlDevice() newBufferWithLength:sizeInBytes options:options];
    CPY_ASSERT(resource.bufferData.mtlBuffer != nil);

    m_workDb.registerResource(
        handle,
        desc.memFlags,
        ResourceGpuState::Default,
        sizeInBytes, 1, 1,
        1, 1,
        // TODO (Apoorva):
        // VulkanResources.cpp and Dx12ResourceCollection.cpp have more complex
        // logic here. Vulkan does this:
        //   resource.counterHandle.valid() ? m_device.countersBuffer() : Buffer()
        // and Dx12 does this:
        //   Buffer counterBuffer;
        //   if (desc.isAppendConsume) {
        //      counterBuffer = m_device.countersBuffer();
        //   }
        // For now, we'll just pass in a new buffer
        Buffer()
    );
    return BufferResult { ResourceResult::Ok, handle.handleId };
}

TextureResult MetalResources::createTexture(const TextureDesc& desc)
{
    std::unique_lock lock(m_mutex);
    ResourceHandle handle;
    MetalResource& resource = m_container.allocate(handle);
    resource.type = MetalResource::Type::Texture;

    // Create the texture descriptor
    MTLTextureDescriptor *texDesc = [[MTLTextureDescriptor alloc] init];
    texDesc.textureType = (MTLTextureType)g_typeTranslations[(int)desc.type];
    if (g_formatTranslations[(int)desc.format] == -1)
    {
        // TODO (Apoorva): Do something better than an assert if a texture
        // format is not supported.
        CPY_ASSERT(false);
    }
    texDesc.pixelFormat = (MTLPixelFormat)g_formatTranslations[(int)desc.format];

    texDesc.width = desc.width;
    texDesc.height = desc.height;
    texDesc.arrayLength = desc.depth;
    texDesc.mipmapLevelCount = desc.mipLevels;
    texDesc.usage = MTLTextureUsageShaderRead;
    if (desc.isRtv)
    {
        texDesc.usage |= MTLTextureUsageShaderWrite;
    }

    resource.textureData.mtlTexture = [m_device.mtlDevice() newTextureWithDescriptor:texDesc];
    CPY_ASSERT(resource.textureData.mtlTexture != nil);

    m_workDb.registerResource(
        handle,
        desc.memFlags,
        ResourceGpuState::Default,
        desc.width, desc.height, desc.depth,
        desc.mipLevels, desc.depth);
        // ^ TODO (Apoorva): desc.depth is used twice. Not sure if this is correct.

    return TextureResult { ResourceResult::Ok, { handle.handleId } };
}

InResourceTableResult MetalResources::createInResourceTable(const ResourceTableDesc& desc)
{
    std::unique_lock lock(m_mutex);
    std::vector<const MetalResource*> resources;

    if (!queryResources(desc.resources, desc.resourcesCount, m_container, resources))
        return InResourceTableResult { ResourceResult::InvalidHandle, InResourceTable(), "Passed an invalid resource to in resource table" };

    ResourceTable handle = createAndFillTable(MetalResourceTable::Type::In, resources.data(), desc.uavTargetMips, &m_tables);
    trackResources(resources.data(), (int)resources.size(), handle, &m_tables);
    m_workDb.registerTable(handle, desc.name.c_str(), desc.resources, desc.resourcesCount, false);
    return InResourceTableResult { ResourceResult::Ok, InResourceTable { handle.handleId } };
}

OutResourceTableResult MetalResources::createOutResourceTable(const ResourceTableDesc& desc)
{
    std::unique_lock lock(m_mutex);
    std::vector<const MetalResource*> resources;

    if (!queryResources(desc.resources, desc.resourcesCount, m_container, resources))
        return OutResourceTableResult { ResourceResult::InvalidHandle, OutResourceTable(), "Passed an invalid resource to out resource table" };

    ResourceTable handle = createAndFillTable(MetalResourceTable::Type::Out, resources.data(), desc.uavTargetMips, &m_tables);
    trackResources(resources.data(), (int)resources.size(), handle, &m_tables);
    m_workDb.registerTable(handle, desc.name.c_str(), desc.resources, desc.resourcesCount, true);
    return OutResourceTableResult { ResourceResult::Ok, OutResourceTable { handle.handleId } };
}

void MetalResources::release(ResourceHandle handle)
{

    CPY_ASSERT(handle.valid());
    if (!handle.valid())
        return;

    CPY_ASSERT(m_container.contains(handle));
    if (!m_container.contains(handle))
        return;

    std::unique_lock lock(m_mutex);
    MetalResource& resource = m_container[handle];
    releaseResourceInternal(handle, resource, &m_tables, &m_workDb);
    m_container.free(handle);
}

}
}
#endif // ENABLE_METAL