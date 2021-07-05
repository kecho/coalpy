#include "Dx12WorkBundle.h"
#include "Dx12Resources.h"

namespace coalpy
{
namespace render
{

bool Dx12WorkBundle::load(const WorkBundle& workBundle)
{
    m_processedLists = workBundle.processedLists;
    for (auto& it : workBundle.states)
        m_states[it.first] = getDx12GpuState(it.second.state);

    return true;
}

}
}
