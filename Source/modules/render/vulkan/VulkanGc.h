#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <coalpy.core/SmartPtr.h>
#include <atomic>
#include "VulkanFencePool.h"

namespace coalpy
{
namespace render
{

class VulkanDevice;

//Async GPU garbage collector
class VulkanGc
{
public:
    VulkanGc(int frequencyMs, VulkanDevice& device);
    ~VulkanGc();

    void start();
    void stop();
    void deferRelease(VkBuffer buffer, VkBufferView bufferView, VkDeviceMemory memory);
    void flush();

private:
    enum class Type
    {    
        None, Buffer, Texture
    };

    struct BufferData
    {
        VkBuffer buffer;
        VkBufferView bufferView;
    };

    struct TextureData
    {
    };

    struct Object
    {
        Type type;
        VkDeviceMemory memory;
        union
        {
            BufferData bufferData;
            TextureData textureData;
        };
    };

    struct ResourceGarbage
    {
        VulkanFenceHandle fenceValue;
        Object object;
    };

    int m_frequency;
    std::atomic<bool> m_active;

    std::mutex m_gcMutex;
    std::thread m_thread;

    VulkanDevice& m_device;
    VulkanFencePool m_fencePool;

    std::queue<Object> m_pendingDeletion;
    std::vector<ResourceGarbage> m_garbage;

    void deleteVulkanObjects(Object& obj);
    void gatherGarbage(int quota = -1);
    void flushDelete(bool blockCpu = false);
    
};

}
}
