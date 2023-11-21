#pragma once

namespace coalpy
{
namespace render
{

class MetalDevice;

enum class WorkType
{
    Graphics,
    Compute,
    Copy,
    Count
};

class MetalQueues
{
public:
    MetalQueues(MetalDevice& device);
    ~MetalQueues();

    id<MTLCommandQueue> cmdQueue(WorkType type) { return (m_containers[(int)type].queue); }
private:

    struct QueueContainer
    {
        id<MTLCommandQueue> queue = nil;
    };

    MetalDevice& m_device;
    QueueContainer m_containers[(int)WorkType::Count];
};

}
}