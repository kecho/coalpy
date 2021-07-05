#pragma once

#include <coalpy.render/Resources.h>
#include "WorkBundleDb.h"
#include <d3d12.h>
#include <unordered_map>
#include <vector>

namespace coalpy
{
namespace render
{

class Dx12WorkBundle
{
public:
    bool load(const WorkBundle& workBundle);

private:
    std::vector<ProcessedList> m_processedLists;
    std::unordered_map<ResourceHandle, D3D12_RESOURCE_STATES> m_states;
};

}
}
