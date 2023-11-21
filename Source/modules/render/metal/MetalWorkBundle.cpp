#include <Config.h>
#if ENABLE_METAL

#include <Metal/Metal.h>
#include "MetalWorkBundle.h"

namespace coalpy
{
namespace render
{

bool MetalWorkBundle::load(const WorkBundle& workBundle)
{
    m_workBundle = workBundle;
    return true;
}

// TODO (Apoorva): Implement
// MetalFenceHandle MetalWorkBundle::execute(CommandList** commandLists, int commandListsCount)
// {
// }
    
}
}

#endif // ENABLE_METAL