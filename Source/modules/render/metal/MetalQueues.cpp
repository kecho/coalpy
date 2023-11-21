#include <Config.h>
#if ENABLE_METAL

#include <Metal/Metal.h>
#include "MetalQueues.h"

namespace coalpy
{
namespace render
{

MetalQueues::MetalQueues(MetalDevice& device)
: m_device(device)
{
}

MetalQueues::~MetalQueues()
{
}

}
}

#endif // ENABLE_METAL