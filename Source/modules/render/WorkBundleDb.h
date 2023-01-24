#pragma once
#include <coalpy.render/CommandDefs.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/AbiCommands.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.core/HandleContainer.h>
#include <stdint.h>
#include <vector>
#include <mutex>
#include <string>
#include <set>
#include <unordered_map>

namespace coalpy
{

namespace render
{

class IDevice;


enum 
{
    ConstantBufferAlignment = 256,
};

struct ResourceDownloadKey
{
    ResourceHandle handle;
    int mipLevel = 0;
    int arraySlice = 0;

    unsigned hash() const
    {
        return (unsigned)(handle.handleId & 0xFFFF) | (unsigned)((mipLevel & 0xF) << 16) | (unsigned)((arraySlice & 0xFF) << 20);
    }

    bool operator <(const ResourceDownloadKey& other) const
    {
        return hash() < other.hash();
    }

    bool operator==(const ResourceDownloadKey& other) const
    {
        return hash() == other.hash();
    }

    bool operator >(const ResourceDownloadKey& other) const
    {
        return hash() > other.hash();
    }

    bool operator <=(const ResourceDownloadKey& other) const
    {
        auto h0 = hash();
        auto h1 = other.hash();
        return h0 <= h1;
    }

    bool operator >=(const ResourceDownloadKey& other) const
    {
        auto h0 = hash();
        auto h1 = other.hash();
        return h0 >= h1;
    }

    bool operator !=(const ResourceDownloadKey& other) const
    {
        return hash() != other.hash();
    }
};

}

}

namespace std
{

template<>
struct hash<coalpy::render::ResourceDownloadKey>
{
    size_t operator()(const coalpy::render::ResourceDownloadKey& k) const
    {
        return k.hash();
    }
};

}

namespace coalpy
{

namespace render
{

enum class ResourceGpuState
{
    Default,
    IndirectArgs,
    Srv,
    Uav,
    Cbv,
    Rtv,
    CopySrc,
    CopyDst,
    Uninitialized,
    Present
};

enum class BarrierType
{
    Immediate,
    Begin,
    End,
    Count
};

struct CommandLocation
{
    int processedListIndex = 0;
    int commandIndex = 0;
    
    uint64_t hash() const { return ((uint64_t)(processedListIndex) << 32) | (uint64_t)(commandIndex); }

    bool operator < (const CommandLocation& other) const
    {
        return hash() < other.hash();
    }
    bool operator <= (const CommandLocation& other) const
    {
        return hash() <= other.hash();
    }
    bool operator > (const CommandLocation& other) const
    {
        return hash() > other.hash();
    }
    bool operator >= (const CommandLocation& other) const
    {
        return hash() >= other.hash();
    }
    bool operator == (const CommandLocation& other) const
    {
        return hash() == other.hash();
    }
    bool operator != (const CommandLocation& other) const
    {
        return hash() != other.hash();
    }
};

struct CommandLocationHasher
{
    std::size_t operator()(const CommandLocation& loc) const
    {
        return loc.hash();
    }
};

struct ResourceBarrier
{
    ResourceHandle resource;
    bool isUav = false; //ignores previous and post states
    CommandLocation srcCmdLocation = {};
    CommandLocation dstCmdLocation = {};
    ResourceGpuState prevState = ResourceGpuState::Default;
    ResourceGpuState postState = ResourceGpuState::Default;
    BarrierType type = BarrierType::Immediate;
};

struct CommandInfo
{
    MemOffset commandOffset = {};
    int uploadBufferOffset = 0;
    int commandDownloadIndex = -1;
    int constantBufferTableOffset = -1;
    int constantBufferCount = 0;
    ResourceMemoryInfo uploadDestinationMemoryInfo;
    std::vector<ResourceBarrier> preBarrier;
    std::vector<ResourceBarrier> postBarrier;
};

struct TableAllocation
{
    int offset = 0;
    int count = 0;
    bool isSampler = false;
};

struct ProcessedList
{
    int listIndex = 0;
    int computeCommandsCount = 0;
    int downloadCommandsCount = 0;
    std::vector<CommandInfo> commandSchedule;
};

struct WorkResourceState
{
    int listIndex;
    int commandIndex;
    ResourceGpuState state;
};

using ResourceStateMap = std::unordered_map<ResourceHandle, WorkResourceState>;
using TableGpuAllocationMap = std::unordered_map<ResourceTable, TableAllocation>;
using ResourceDownloadSet  = std::set<ResourceDownloadKey>;

struct WorkBundle
{
    std::vector<ProcessedList> processedLists;
    ResourceStateMap states;

    int totalTableSize = 0;
    int totalConstantBuffers = 0;
    int totalUploadBufferSize = 0;
    int totalSamplers = 0;

    ResourceDownloadSet resourcesToDownload;
    TableGpuAllocationMap tableAllocations;
};

struct WorkTableInfo
{
    bool isUav;
    std::string name;
    std::vector<ResourceHandle> resources;
};

struct WorkResourceInfo
{
    MemFlags memFlags = {};
    ResourceGpuState gpuState = ResourceGpuState::Default;
    Buffer counterBuffer;
    int sizeX = 0;
    int sizeY = 0;
    int sizeZ = 0;
    int mipLevels = 1;
    int arraySlices = 1;
};

using WorkTableInfos = std::unordered_map<ResourceTable,  WorkTableInfo>;
using WorkResourceInfos = std::unordered_map<ResourceHandle, WorkResourceInfo>;


enum WorkBundleDbFlags 
{
    WorkBundleDbFlags_None = 0,
    WorkBundleDbFlags_SetupTablePreallocations = 1 << 0
};

class WorkBundleDb
{
public:
    WorkBundleDb(IDevice& device, WorkBundleDbFlags flags = WorkBundleDbFlags_None) : m_device(device), m_flags(flags) {}
    ~WorkBundleDb() {}

    ScheduleStatus build(CommandList** lists, int listCount);
    void release(WorkHandle);

    void registerTable(ResourceTable table, const char* name, const ResourceHandle* handles, int handleCounts, bool isUav);
    void unregisterTable(ResourceTable table);
    void clearAllTables() { m_tables.clear(); }

    void registerResource(
        ResourceHandle handle,
        MemFlags flags,
        ResourceGpuState initialState,
        int sizeX, int sizeY, int sizeZ,
        int mipLevels, int arraySlices,
        Buffer counterBuffer = Buffer());

    void unregisterResource(ResourceHandle handle);
    void clearAllResources() { m_resources.clear(); }

    bool writeResourceStates(WorkHandle handle);

    void lock() { m_workMutex.lock(); }
    WorkBundle& unsafeGetWorkBundle(WorkHandle handle) { return m_works[handle]; }
    WorkResourceInfos& resourceInfos() { return m_resources; }
    WorkTableInfos& tableInfos() { return m_tables; }
    void unlock() { m_workMutex.unlock(); }

private:
    std::mutex m_workMutex;

    IDevice& m_device;
    HandleContainer<WorkHandle, WorkBundle> m_works;

    WorkTableInfos m_tables;
    WorkResourceInfos m_resources;
    WorkBundleDbFlags m_flags;
};

}

} 
