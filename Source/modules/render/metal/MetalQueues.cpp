#include "Config.h"
#if ENABLE_METAL

#include <Metal/Metal.h>
#include "MetalQueues.h"
#include "MetalDevice.h"

namespace coalpy
{
namespace render
{

MetalQueues::MetalQueues(MetalDevice& device)
: m_device(device)
{
    for (int i = 0u; i < (int)WorkType::Count; ++i)
    {
        m_containers[i].queue = [m_device.mtlDevice() newCommandQueue];
    }
}

MetalQueues::~MetalQueues()
{
}

}
}

#endif // ENABLE_METAL