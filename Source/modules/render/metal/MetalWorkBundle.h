#pragma once
#include <stdint.h>
#include <vector>
#include "WorkBundleDb.h"

namespace coalpy
{
namespace render
{

class CommandList;
class MetalDevice;

struct MetalResourceDownloadState
{
};

using MetalDownloadResourceMap = std::unordered_map<ResourceDownloadKey, MetalResourceDownloadState>;

class MetalWorkBundle
{
public:
    MetalWorkBundle(MetalDevice& device) : m_device(device) {}
    bool load(const WorkBundle& workBundle);
    // MetalFenceHandle execute(CommandList** commandLists, int commandListsCount);
    // void getDownloadResourceMap(MetalDownloadResourceMap& downloadMap);

private:
    WorkBundle m_workBundle;
    MetalDevice& m_device;
    std::vector<MetalResourceDownloadState> m_downloadStates;
};

}
}
