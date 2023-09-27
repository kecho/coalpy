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

MetalResources::MetalResources(MetalDevice& device, WorkBundleDb& workDb) :
    m_device(device),
    m_workDb(workDb)
{
}

MetalResources::~MetalResources()
{
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

}
}
#endif // ENABLE_METAL