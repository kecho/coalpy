#include "VulkanDescriptorSetPools.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include <coalpy.core/Assert.h>
#include <Config.h>
#include <iostream>

namespace coalpy
{
namespace render
{

VulkanDescriptorSetPools::VulkanDescriptorSetPools(VulkanDevice& device)
: m_device(device)
{
}

VulkanDescriptorSetPools::~VulkanDescriptorSetPools()
{
    m_pools.forEach([this](VulkanDescriptorPoolHandle handle, PoolState& ps)
    {
        vkDestroyDescriptorPool(m_device.vkDevice(), ps.pool, nullptr);
    });
}

VulkanDescriptorTable VulkanDescriptorSetPools::allocate(const VkDescriptorSetLayout& layout)
{
    if (m_validPools.empty())
        allocateNewPool();

    VkDescriptorSetAllocateInfo allocationInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
    allocationInfo.descriptorPool = {};
    allocationInfo.descriptorSetCount = 1;
    allocationInfo.pSetLayouts = &layout;

    while (!m_validPools.empty())
    {
        VkDescriptorSet descriptorSet = {};
        PoolState& poolState = m_pools[m_validPools.top()];
        CPY_ASSERT(!poolState.mightBeFull);

        allocationInfo.descriptorPool = poolState.pool;
        if (vkAllocateDescriptorSets(m_device.vkDevice(), &allocationInfo, &descriptorSet) != VK_SUCCESS)
        {
            poolState.mightBeFull = true;
            m_validPools.pop();
        }
        else
            return VulkanDescriptorTable { m_validPools.top() , descriptorSet };

        if (m_validPools.empty())
            allocateNewPool();
    }

    CPY_ASSERT_MSG(false, "Failed allocating descriptor pools.");
    return {};
}

void VulkanDescriptorSetPools::allocateNewPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 64 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 64 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr };
    poolInfo.flags = {};
    if ((m_device.enabledDeviceExts() & asFlag(VulkanDeviceExtensions::MutableDescriptor)) != 0)
        poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_VALVE;
    poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 32;
    poolInfo.poolSizeCount = (int)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    
    VkDescriptorPool newPool = {};
    if (vkCreateDescriptorPool(m_device.vkDevice(), &poolInfo, nullptr, &newPool) == VK_SUCCESS)
    {
        VulkanDescriptorPoolHandle newHandle;
        PoolState& newState = m_pools.allocate(newHandle);
        newState = { newPool, false };
        m_validPools.push(newHandle);
    }
}

void VulkanDescriptorSetPools::free(VulkanDescriptorTable table)
{
    CPY_ASSERT(m_pools.contains(table.parentPool));
    if (!m_pools.contains(table.parentPool))
        return;

    PoolState& ps = m_pools[table.parentPool];
    VK_OK(vkFreeDescriptorSets(m_device.vkDevice(), ps.pool, 1, &table.descriptors));
    if (ps.mightBeFull)
    {
        ps.mightBeFull = false;
        m_validPools.push(table.parentPool);
    }
}

}
}
