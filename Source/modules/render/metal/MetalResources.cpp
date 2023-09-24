#include "Config.h"
#if ENABLE_METAL

#include "MetalResources.h"
#include "MetalDevice.h"

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

    // TODO (Apoorva): This code is copied from VulkanResources::createBuffer().
    // Check if this is also a limitation of Metal.
    if ((desc.memFlags & MemFlag_GpuWrite) != 0 && (desc.memFlags & MemFlag_GpuRead) != 0 && (specialFlags & ResourceSpecialFlag_CpuReadback) != 0)
        return BufferResult  { ResourceResult::InvalidHandle, Buffer(), "Unsupported special flags combined with mem flags." };
    // TODO (Apoorva): This code is copied from VulkanResources::createBuffer().
    // Check if this is also a limitation of Metal.
    if (desc.isAppendConsume && desc.type != BufferType::Structured)
        return BufferResult { ResourceResult::InvalidParameter, Buffer(), "Append consume buffers can only be of type Structured." };
}

}
}
#endif // ENABLE_METAL