#include "Config.h"
#include "VulkanGc.h"
#include "VulkanDevice.h"
#include "VulkanQueues.h"
#include <coalpy.core/Assert.h>
#include <chrono>

namespace coalpy
{
namespace render
{

VulkanGc::VulkanGc(int frequencyMs, VulkanDevice& device)
: m_device(device), m_fencePool(device), m_frequency(frequencyMs), m_active(false)
{
}

VulkanGc::~VulkanGc()
{
    if (m_active)
        stop();

    gatherGarbage();
    flushDelete(true /*wait on GPU by blocking CPU*/);
}

void VulkanGc::gatherGarbage(int quota)
{
    std::vector<Object> tmpObjects;
    
    {
        std::unique_lock lock(m_gcMutex);
        while (!m_pendingDeletion.empty() && (quota != 0))
        {
            Object obj = m_pendingDeletion.front();
            m_pendingDeletion.pop();
            tmpObjects.push_back(obj);
            --quota;
        }
    }

    if (!tmpObjects.empty())
    {
        VulkanFenceHandle fenceVal = m_fencePool.allocate(); 
        for (auto& obj : tmpObjects)
        {
            ResourceGarbage g { fenceVal, obj };
            m_fencePool.addRef(fenceVal);
            m_garbage.push_back(g);
        }
        VkQueue queue = m_device.queues().cmdQueue(WorkType::Graphics);
        VkFence fence = m_fencePool.get(fenceVal);
        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
        submitInfo.commandBufferCount = 0;
        submitInfo.pCommandBuffers = nullptr;
        VK_OK(vkQueueSubmit(queue, 1u, &submitInfo, fence));
        m_fencePool.free(fenceVal);
    }
}

void VulkanGc::flushDelete(bool waitOnCpu)
{
    if (m_garbage.empty())
        return;

    int deleted = 0;
    for (auto& g : m_garbage)
    {
        if (g.object.type == Type::None)
        {
            ++deleted;
            continue;
        }

        if (waitOnCpu)
        {
            m_fencePool.waitOnCpu(g.fenceValue);
        }

        m_fencePool.updateState(g.fenceValue);
        if (m_fencePool.isSignaled(g.fenceValue))
        {
            m_fencePool.free(g.fenceValue);
            g.fenceValue = VulkanFenceHandle();
            deleteVulkanObjects(g.object);
            g.object.type = Type::None;
            ++deleted;
        }
    }
    
    if (deleted == (int)m_garbage.size())
        m_garbage.clear();
}

void VulkanGc::deleteVulkanObjects(Object& obj)
{
    if (obj.memory)
    {
        vkFreeMemory(m_device.vkDevice(), obj.memory, nullptr);
    }

    switch (obj.type)
    {
    case Type::Buffer:
        {
            BufferData& bufferData = obj.bufferData;
            if (bufferData.bufferView) { 
                vkDestroyBufferView(m_device.vkDevice(), bufferData.bufferView, nullptr); }
            if (bufferData.buffer) {
                vkDestroyBuffer(m_device.vkDevice(), bufferData.buffer, nullptr); }
        }
        break;
    case Type::Texture:
        {
            TextureData& textureData = obj.textureData;
            if (textureData.image)
                vkDestroyImage(m_device.vkDevice(), textureData.image, nullptr);

            for (int i = 0; i < textureData.imageViewsCount; ++i)
                if (textureData.imageViews[i])
                    vkDestroyImageView(m_device.vkDevice(), textureData.imageViews[i], nullptr);
        }
        break;
    case Type::ComputePipeline:
        {
            ComputePipelineData& data = obj.computeData;
            if (data.shaderModule)
                vkDestroyShaderModule(m_device.vkDevice(), data.shaderModule, nullptr);

            if (data.pipeline)
                vkDestroyPipeline(m_device.vkDevice(), data.pipeline, nullptr);

            if (data.pipelineLayout)
                vkDestroyPipelineLayout(m_device.vkDevice(), data.pipelineLayout, nullptr);
        }
        break;
    default:
        return;
    }
}

void VulkanGc::deferRelease(VkImage image, VkImageView* uavs, int uavCounts, VkImageView srv, VkDeviceMemory memory)
{
    Object obj;
    obj.type = Type::Texture;
    obj.memory = memory;
    auto& data = obj.textureData;
    data.image = image;
    CPY_ASSERT((uavCounts + (srv ? 1 : 0)) <= (VulkanMaxMips + 1));
    data.imageViewsCount = 0;
    for (int i = 0; i < uavCounts; ++i)
    {
        if (uavs[i])
            data.imageViews[data.imageViewsCount++] = uavs[i];
    }

    if (srv)
        data.imageViews[data.imageViewsCount++] = srv;

    {
        std::unique_lock lock(m_gcMutex);
        m_pendingDeletion.push(obj);
    }
}

void VulkanGc::deferRelease(VkBuffer buffer, VkBufferView bufferView, VkDeviceMemory memory)
{
    Object obj;
    obj.type = Type::Buffer;
    obj.memory = memory;
    auto& data = obj.bufferData;
    data.buffer = buffer;
    data.bufferView = bufferView;
    
    {
        std::unique_lock lock(m_gcMutex);
        m_pendingDeletion.push(obj);
    }
}

void VulkanGc::deferRelease(VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkShaderModule shaderModule)
{
    Object obj;
    obj.type = Type::ComputePipeline;
    obj.memory = VK_NULL_HANDLE;
    auto& data = obj.computeData;
    data.pipelineLayout = pipelineLayout;
    data.pipeline = pipeline;
    data.shaderModule = shaderModule;
    
    {
        std::unique_lock lock(m_gcMutex);
        m_pendingDeletion.push(obj);
    }
}

void VulkanGc::start()
{
    m_active = true;
    m_thread = std::thread([this]()
    {
        while (m_active)
        {
            gatherGarbage(128);
            flushDelete(false /*poll on GPU*/);

            std::this_thread::sleep_for(std::chrono::milliseconds(m_frequency));
        }
    });
}

void VulkanGc::stop()
{
    m_active = false;
    m_thread.join();
}

void VulkanGc::flush()
{
}

}
}
