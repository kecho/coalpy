#pragma once

namespace coalpy
{
namespace render
{

class MetalDevice;

class MetalQueues
{
public:
    MetalQueues(MetalDevice& device);
    ~MetalQueues();
private:
    MetalDevice& m_device;
};

}
}