#include "WorkBundleDb.h"
#include <coalpy.core/Assert.h>
#include <sstream>

namespace coalpy
{

namespace render
{

namespace 
{


struct WorkResourceState
{
    int listIndex;
    int commandIndex;
    ResourceGpuState state;
};

using ResourceStateMap = std::unordered_map<ResourceHandle, WorkResourceState>;

struct WorkBuildContext
{
    //current context info (which list and command we are at)
    int listIndex;
    int currentCommandIndex;
    MemOffset command;

    //output of the context, error and mutable states
    ScheduleErrorType errorType;
    std::string errorMsg;
    ResourceStateMap states;
    std::vector<ProcessedList> processedList;

    //immutable data, current state of tables and resources in gpu
    const WorkResourceInfos* resourceInfos = nullptr;
    const WorkTableInfos* tableInfos = nullptr;
};

bool transitionResource(
    ResourceHandle resource,
    ResourceGpuState newState,
    WorkBuildContext context)
{
    const WorkResourceInfos& resourceInfos = *context.resourceInfos;
    auto it = context.states.find(resource);
    bool canSplitBarrier = false;
    WorkResourceState* currState = nullptr;

    if (it != context.states.end())
    {
        WorkResourceState* currState = &it->second;
        canSplitBarrier = currState->listIndex != context.listIndex ||
            (context.currentCommandIndex - currState->commandIndex) >= 2;
    }

    if (currState != nullptr && canSplitBarrier)
    {
        if (currState->state != newState)
        {
            {
                auto& srcList = context.processedList[currState->listIndex];
                CPY_ASSERT(srcList.listIndex == currState->listIndex);
                CommandInfo& srcCmd = srcList.commandSchedule[currState->commandIndex];
                srcCmd.postBarrier.emplace_back();
                auto& beginBarrier = srcCmd.postBarrier.back();
                beginBarrier.resource = resource;
                beginBarrier.prevState = currState->state;
                beginBarrier.postState = newState;
                beginBarrier.type = BarrierType::Begin;
            }

            {
                auto& dstList = context.processedList[context.listIndex];
                CPY_ASSERT(dstList.listIndex == context.listIndex);
                CommandInfo& dstCmd = dstList.commandSchedule[context.currentCommandIndex];
                dstCmd.preBarrier.emplace_back();
                auto& endBarrier = dstCmd.preBarrier.back();
                endBarrier.resource = resource;
                endBarrier.prevState = currState->state;
                endBarrier.postState = newState;
                endBarrier.type = BarrierType::End;
            }

            currState->state = newState;
        }
        
        currState->listIndex = context.listIndex;
        currState->commandIndex = context.currentCommandIndex;
    }
    else
    {
        ResourceGpuState prevState = {};
        if (currState)
        {
            prevState = currState->state;
        }
        else
        {
            auto prevStateIt = resourceInfos.find(resource);
            if (prevStateIt == resourceInfos.end())
            {
                std::stringstream ss;
                ss << "Could not find registered resource id " << resource.handleId;
                context.errorMsg = ss.str();
                context.errorType = ScheduleErrorType::ResourceStateNotFound;
                return false;
            }

            prevState = prevStateIt->second.gpuState;
        }

        if (currState == nullptr)
        {
            WorkResourceState newStateRecord;
            newStateRecord.listIndex = context.listIndex;
            newStateRecord.commandIndex = context.currentCommandIndex;
            newStateRecord.state = newState;
            context.states[resource] = newStateRecord;
        }

        if (prevState != newState)
        {
            ResourceBarrier newBarrier;
            newBarrier.resource = resource;
            newBarrier.prevState = prevState; 
            newBarrier.postState = newState; 
            newBarrier.type = BarrierType::Immediate;
            context.processedList[context.listIndex]
                .commandSchedule[context.currentCommandIndex]
                .preBarrier.push_back(newBarrier);
        }
    }

    return true;
}

bool transitionTable(
    ResourceTable table,
    WorkBuildContext& context)
{
    const WorkResourceInfos& resourceInfos = *context.resourceInfos;
    const WorkTableInfos& tableInfos = *context.tableInfos;
    auto tableInfIt = tableInfos.find(table);
    if (tableInfIt == tableInfos.end())
    {
        std::stringstream ss;
        ss << "Could not find table information for table id: " << table.handleId;
        context.errorMsg = ss.str();
        context.errorType = ScheduleErrorType::BadTableInfo;
        return false;
    }

    const WorkTableInfo& tableInfo = tableInfIt->second;
    auto newState = tableInfo.isUav ? ResourceGpuState::Uav : ResourceGpuState::Srv;
    for (auto r : tableInfo.resources)
    {
        if (!transitionResource(r, newState, context))
            return false;
    }

    return true;
}

bool commitResourceStates(const ResourceStateMap& input, WorkResourceInfos& resourceInfos)
{
    for (auto& it : input)
    {
        auto outIt = resourceInfos.find(it.first);
        if (outIt == resourceInfos.end())
            return false;

        resourceInfos[it.first].gpuState = it.second.state;
    }

    return true;
} 

bool processCompute(const AbiComputeCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    {
        const InResourceTable* inTables = cmd->inResourceTables.data(data);
        for (int i = 0; i < cmd->inResourceTablesCounts; ++i)
        {
            if (!transitionTable(inTables[i], context))
                return false;
        }
    }


    {
        const OutResourceTable* outTables = cmd->outResourceTables.data(data);
        for (int i = 0; i < cmd->outResourceTablesCounts; ++i)
        {
            if (!transitionTable(outTables[i], context))
                return false;
        }
    }

    {
        const Buffer* cbuffers = cmd->constants.data(data);
        for (int i = 0; i < cmd->constantCounts; ++i)
        {
            if (!transitionResource(cbuffers[i], ResourceGpuState::Cbv, context))
                return false;
        }
    }

    return true;
}

bool processCopy(const AbiCopyCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    if (!transitionResource(cmd->source, ResourceGpuState::CopySrc, context))
        return false;

    if (!transitionResource(cmd->destination, ResourceGpuState::CopyDst, context))
        return false;

    return true;
}

bool processUpload(const AbiUploadCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    if (!transitionResource(cmd->destination, ResourceGpuState::CopyDst, context))
        return false;

    return true;
}

bool processDownload(const AbiDownloadCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    if (!transitionResource(cmd->source, ResourceGpuState::CopySrc, context))
        return false;

    return false;
}

void ParseCommandList(const unsigned char* data, WorkBuildContext& context)
{
    MemOffset offset = 0ull;
    const auto& header = *((AbiCommandListHeader*)data);
    CPY_ASSERT((AbiCmdTypes)header.sentinel == AbiCmdTypes::CommandListSentinel);

    offset += sizeof(AbiCommandListHeader);

    int listIndex = context.listIndex;
    context.processedList[listIndex].listIndex = context.listIndex;
    context.processedList[listIndex].commandSchedule = {};

    bool finished = false;
    int currentCommandIndex = 0;
    while (!finished)
    {
        auto currentSentinel = (AbiCmdTypes)(*((int*)(data + offset)));
        context.command = offset;
        context.currentCommandIndex = currentCommandIndex;
        if (currentSentinel != AbiCmdTypes::CommandListEndSentinel)
        {
            context.processedList[listIndex].commandSchedule.emplace_back();
            auto& cmdInfo = context.processedList[listIndex].commandSchedule.back();
            cmdInfo.commandOffset = offset;
        }
            
        switch (currentSentinel)
        {
            case AbiCmdTypes::CommandListEndSentinel:
                finished = true;
                offset += sizeof(int);
                break;
            case AbiCmdTypes::Compute:
                finished = processCompute((const AbiComputeCmd*)(data + offset), data, context);
                offset += sizeof(AbiComputeCmd);
                break;
            case AbiCmdTypes::Copy:
                finished = processCopy((const AbiCopyCmd*)(data + offset), data, context);
                offset += sizeof(AbiCopyCmd);
                break;
            case AbiCmdTypes::Upload:
                finished = processUpload((const AbiUploadCmd*)(data + offset), data, context);
                offset += sizeof(AbiUploadCmd);
                break;
            case AbiCmdTypes::Download:
                finished = processDownload((const AbiDownloadCmd*)(data + offset), data, context);
                offset += sizeof(AbiDownloadCmd);
                break;
            default:
            {
                std::stringstream ss;
                ss << "Unrecognized command sentinel parsed: " << (int)currentSentinel;
                context.errorType = ScheduleErrorType::CorruptedCommandListSentinel;
                context.errorMsg = ss.str();
                finished = true;
                break;
            }
        }
        ++currentCommandIndex;
    }
}

}

ScheduleStatus WorkBundleDb::build(CommandList** lists, int listCount)
{
    WorkHandle handle;
    WorkBundle newBundle;

    //todo build the bundle here
    {
        std::unique_lock lock(m_workMutex);
        m_works.allocate(handle);
    }

    return ScheduleStatus { handle, ScheduleErrorType::Ok, "" };
}

void WorkBundleDb::release(WorkHandle handle)
{
    std::unique_lock lock(m_workMutex);
    CPY_ASSERT(handle.valid());
    CPY_ASSERT(m_works.contains(handle));
    if (!m_works.contains(handle))
        return;

    m_works.free(handle);
}

void WorkBundleDb::registerTable(ResourceTable table, const ResourceHandle* handles, int handleCounts, bool isUav)
{
    auto& newInfo = m_tables[table];
    newInfo.isUav = isUav;
    newInfo.resources.assign(handles, handles + handleCounts);
}

void WorkBundleDb::unregisterTable(ResourceTable table)
{
    m_tables.erase(table);
}

void WorkBundleDb::registerResource(ResourceHandle handle, ResourceGpuState initialState)
{
    auto& resInfo = m_resources[handle];
    resInfo.gpuState = initialState;
}

void WorkBundleDb::unregisterResource(ResourceHandle handle)
{
    m_resources.erase(handle);
}

}

}
