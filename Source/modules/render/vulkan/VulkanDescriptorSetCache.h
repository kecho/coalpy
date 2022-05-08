#pragma once

namespace coalpy
{
namespace render
{

class VulkanDevice;

class VulkanDescriptorSetCache
{
public:

    VulkanDescriptorSetCache(VulkanDevice& device);
    ~VulkanDescriptorSetCache();

private:
    VulkanDevice& m_device;
};

}
}
