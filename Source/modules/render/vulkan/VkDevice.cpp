#include <Config.h>
#if ENABLE_VULKAN

#include "VkDevice.h"
#include "VkShaderDb.h"

namespace coalpy
{
namespace render
{

VkDevice::VkDevice(const DeviceConfig& config)
: TDevice<VkDevice>(config),
  m_shaderDb(nullptr)
{
    if (config.shaderDb)
    {
        m_shaderDb = static_cast<VkShaderDb*>(config.shaderDb);
        CPY_ASSERT_MSG(m_shaderDb->parentDevice() == nullptr, "shader database can only belong to 1 and only 1 device");
        m_shaderDb->setParentDevice(this);
    }

    m_info = {};
    m_info.valid = true;
}

VkDevice::~VkDevice()
{
    if (m_shaderDb && m_shaderDb->parentDevice() == this)
        m_shaderDb->setParentDevice(nullptr);
}

void VkDevice::enumerate(std::vector<DeviceInfo>& outputList)
{
}

TextureResult VkDevice::createTexture(const TextureDesc& desc)
{
    return TextureResult();
}

TextureResult VkDevice::recreateTexture(Texture texture, const TextureDesc& desc)
{
    return TextureResult();
}

BufferResult  VkDevice::createBuffer (const BufferDesc& config)
{
    return BufferResult();
}

SamplerResult VkDevice::createSampler (const SamplerDesc& config)
{
    return SamplerResult();
}

InResourceTableResult VkDevice::createInResourceTable  (const ResourceTableDesc& config)
{
    return InResourceTableResult();
}

OutResourceTableResult VkDevice::createOutResourceTable (const ResourceTableDesc& config)
{
    return OutResourceTableResult();
}

SamplerTableResult  VkDevice::createSamplerTable (const ResourceTableDesc& config)
{
    return SamplerTableResult();
}

void VkDevice::getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo)
{
    memInfo = ResourceMemoryInfo();
}

WaitStatus VkDevice::waitOnCpu(WorkHandle handle, int milliseconds)
{
    return WaitStatus();
}

DownloadStatus VkDevice::getDownloadStatus(WorkHandle bundle, ResourceHandle handle, int mipLevel, int arraySlice)
{
    return DownloadStatus();
}

void VkDevice::release(ResourceHandle resource)
{
}

void VkDevice::release(ResourceTable table)
{
}

SmartPtr<IDisplay> VkDevice::createDisplay(const DisplayConfig& config)
{
    return nullptr;
}

void VkDevice::internalReleaseWorkHandle(WorkHandle handle)
{
}

ScheduleStatus VkDevice::internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle)
{
    return ScheduleStatus();
}

}
}

#endif
