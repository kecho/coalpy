#pragma once

#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <stack>
#include <vulkan/vulkan.h>

namespace coalpy
{
namespace render
{

class VulkanDevice;

struct VulkanDescriptorPoolHandle  : public GenericHandle<unsigned int> {};

struct VulkanDescriptorTable
{
    VulkanDescriptorPoolHandle parentPool;
    VkDescriptorSet descriptors = {};
};

class VulkanDescriptorSetPools
{
public:

    VulkanDescriptorSetPools(VulkanDevice& device);
    ~VulkanDescriptorSetPools();
    
    VulkanDescriptorTable allocate(
        const VkDescriptorSetLayout& layout);

    void free(VulkanDescriptorTable table);

private:
    void allocateNewPool();

    struct PoolState
    {
        VkDescriptorPool pool = {};
        bool mightBeFull = false;
    };

    std::stack<VulkanDescriptorPoolHandle> m_validPools;

    VulkanDevice& m_device;
    HandleContainer<VulkanDescriptorPoolHandle, PoolState> m_pools;
};

}
}
