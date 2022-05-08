#include "VulkanDescriptorSetCache.h"
#include "VulkanDevice.h"

namespace coalpy
{
namespace render
{

VulkanDescriptorSetCache::VulkanDescriptorSetCache(VulkanDevice& device)
: m_device(device)
{
}

VulkanDescriptorSetCache::~VulkanDescriptorSetCache()
{
}

}
}
