#include "WorkBundleDb.h"
#include <coalpy.core/Assert.h>
#include <coalpy.render/IDevice.h>
#include <sstream>

namespace coalpy
{

namespace render
{

namespace 
{

enum 
{
    ConstantBufferAlignment = 256,
};

struct WorkBuildContext
{
    IDevice* device = nullptr;
    //current context info (which list and command we are at)
    int listIndex = -1;
    int currentCommandIndex = -1;
    MemOffset command = 0;

    //output of the context, error and mutable states
    ScheduleErrorType errorType = ScheduleErrorType::Ok;
    std::string errorMsg;
    ResourceStateMap states;
    ResourceDownloadSet resourcesToDownload;
    TableGpuAllocationMap tableAllocations;
    std::vector<ProcessedList> processedList;
    int totalTableSize = 0;
    int totalConstantBuffers = 0;
    int totalUploadBufferSize = 0;
    int totalSamplers = 0;

    //immutable data, current state of tables and resources in gpu
    const WorkResourceInfos* resourceInfos = nullptr;
    const WorkTableInfos* tableInfos = nullptr;

    ProcessedList& currentListInfo()
    {
        return processedList[listIndex];
    }

    CommandInfo& currentCommandInfo()
    {
        return currentListInfo().commandSchedule[currentCommandIndex];
    }
};

bool transitionResource(
    ResourceHandle resource,
    ResourceGpuState newState,
    WorkBuildContext& context)
{
    const WorkResourceInfos& resourceInfos = *context.resourceInfos;
    auto it = context.states.find(resource);
    bool canSplitBarrier = false;
    WorkResourceState* currState = nullptr;

    if (it != context.states.end())
    {
        currState = &it->second;
        canSplitBarrier = currState->listIndex != context.listIndex ||
            (context.currentCommandIndex - currState->commandIndex) >= 2;
    }

    if (currState != nullptr && canSplitBarrier)
    {
        auto& srcList = context.processedList[currState->listIndex];
        CPY_ASSERT(srcList.listIndex == currState->listIndex);
        auto& dstList = context.processedList[context.listIndex];
        CPY_ASSERT(dstList.listIndex == context.listIndex);
        CommandInfo& srcCmd = srcList.commandSchedule[currState->commandIndex];
        CommandInfo& dstCmd = dstList.commandSchedule[context.currentCommandIndex];

        auto prevState = currState->state;
        if (prevState != newState)
        {
            {
                srcCmd.postBarrier.emplace_back();
                auto& beginBarrier = srcCmd.postBarrier.back();
                beginBarrier.resource = resource;
                beginBarrier.prevState = prevState;
                beginBarrier.postState = newState;
                beginBarrier.type = BarrierType::Begin;
            }

            {
                dstCmd.preBarrier.emplace_back();
                auto& endBarrier = dstCmd.preBarrier.back();
                endBarrier.resource = resource;
                endBarrier.prevState = prevState;
                endBarrier.postState = newState;
                endBarrier.type = BarrierType::End;
            }

            currState->state = newState;
        }

        if (prevState == ResourceGpuState::Uav && newState == ResourceGpuState::Uav)
        {
            {
                srcCmd.postBarrier.emplace_back();
                auto& beginBarrier = srcCmd.postBarrier.back();
                beginBarrier.resource = resource;
                beginBarrier.isUav = true;
                beginBarrier.type = BarrierType::Begin;
            }

            {
                dstCmd.preBarrier.emplace_back();
                auto& endBarrier = dstCmd.preBarrier.back();
                endBarrier.resource = resource;
                endBarrier.isUav = true;
                endBarrier.type = BarrierType::End;
            }
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
            auto it = context.states.insert(std::pair<ResourceHandle, WorkResourceState>(resource, newStateRecord));
            currState = &it.first->second;
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
            currState->state = newState;
        }

        if (prevState == ResourceGpuState::Uav && newState == ResourceGpuState::Uav)
        {
            ResourceBarrier newBarrier;
            newBarrier.resource = resource;
            newBarrier.isUav = true; 
            newBarrier.type = BarrierType::Immediate;
            context.processedList[context.listIndex]
                .commandSchedule[context.currentCommandIndex]
                .preBarrier.push_back(newBarrier);
        }

        currState->commandIndex = context.currentCommandIndex;
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

bool processTable(
    ResourceTable table,
    WorkBuildContext& context)
{
    if (!transitionTable(table, context))
        return false;

    if (context.tableAllocations.find(table) != context.tableAllocations.end())
        return true;

    const WorkTableInfos& tableInfos = *context.tableInfos;
    const auto& tableInfoIt = tableInfos.find(table);
    if (tableInfoIt == tableInfos.end())
        return false;

    TableAllocation& allocation = context.tableAllocations[table];
    allocation.offset = context.totalTableSize;
    allocation.count = tableInfoIt->second.resources.size();
    context.totalTableSize += allocation.count;
    return true;
}

bool processSamplerTable(
    ResourceTable table,
    WorkBuildContext& context)
{
    const WorkTableInfos& tableInfos = *context.tableInfos;
    const auto& tableInfoIt = tableInfos.find(table);
    if (tableInfoIt == tableInfos.end())
        return false;

    TableAllocation& allocation = context.tableAllocations[table];
    allocation.offset = context.totalSamplers;
    allocation.count = tableInfoIt->second.resources.size();
    allocation.isSampler = true;
    context.totalSamplers += allocation.count;
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
            if (!processTable(inTables[i], context))
                return false;
        }
    }

    {
        const OutResourceTable* outTables = cmd->outResourceTables.data(data);
        for (int i = 0; i < cmd->outResourceTablesCounts; ++i)
        {
            if (!processTable(outTables[i], context))
                return false;
        }
    }

    {
        const SamplerTable* samplerTables = cmd->samplerTables.data(data);
        for (int i = 0; i < cmd->samplerTablesCounts; ++i)
        {
            if (!processSamplerTable(samplerTables[i], context))
                return false;
        }
    }

    {
        const Buffer* cbuffers = cmd->constants.data(data);
        CommandInfo& cmdInfo = context.currentCommandInfo();
        if (cmd->inlineConstantBufferSize > 0)
        {
            //TODO: hack, dx12 requires aligned buffers to be 256.
            cmdInfo.uploadBufferOffset = context.totalUploadBufferSize;
            int alignedBufferOffset = ((cmdInfo.uploadBufferOffset + (ConstantBufferAlignment - 1)) / ConstantBufferAlignment) * ConstantBufferAlignment;
            int alignedBufferSize = ((cmd->inlineConstantBufferSize + (ConstantBufferAlignment - 1))/ConstantBufferAlignment) * ConstantBufferAlignment;
            int padding = alignedBufferOffset - context.totalUploadBufferSize;
            cmdInfo.uploadBufferOffset += padding;
            context.totalUploadBufferSize += alignedBufferSize + padding;

            cmdInfo.constantBufferTableOffset = context.totalConstantBuffers;
            ++context.totalConstantBuffers;
        }
        else
        {
            for (int i = 0; i < cmd->constantCounts; ++i)
            {
                if (!transitionResource(cbuffers[i], ResourceGpuState::Cbv, context))
                    return false;
            }

            cmdInfo.constantBufferCount = cmd->constantCounts;
            cmdInfo.constantBufferTableOffset = context.totalConstantBuffers;
            context.totalConstantBuffers += cmdInfo.constantBufferCount;
        }
    }

    if (cmd->isIndirect)
    {
        if (!cmd->indirectArguments.valid())
        {
            std::stringstream ss;
            ss << "Indirect argument buffer is not valid for indirect dispatch command ";
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::InvalidResource;
            return false;
        }

        if (!transitionResource(cmd->indirectArguments, ResourceGpuState::IndirectArgs, context))
            return false;
    }

    ++context.currentListInfo().computeCommandsCount;

    return true;
}

bool processCopy(const AbiCopyCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    if (!transitionResource(cmd->source, ResourceGpuState::CopySrc, context))
        return false;

    if (!transitionResource(cmd->destination, ResourceGpuState::CopyDst, context))
        return false;

    auto fitsInCopyCmd = [&context, &cmd](ResourceHandle handle, const char* resourceTypeName, int offsetX, int offsetY, int offsetZ, int mipLevel)
    {
        auto it = context.resourceInfos->find(handle);
        if (it == context.resourceInfos->end())
        {
            std::stringstream ss;
            ss << "Invalid resource passed on copy command";
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::InvalidResource;
            return false;
        }

        const WorkResourceInfo& resourceInfo = it->second;
        int remainingSizeX = resourceInfo.sizeX - offsetX;
        int remainingSizeY = resourceInfo.sizeY - offsetY;
        int remainingSizeZ = resourceInfo.sizeZ - offsetZ;
        if (cmd->sizeX > remainingSizeX || cmd->sizeY > remainingSizeY || cmd->sizeZ > remainingSizeZ)
        {
            std::stringstream ss;
            ss << resourceTypeName << " in copy command is outside of bounds. Resource sizes are ["
               << resourceInfo.sizeX << ", " << resourceInfo.sizeY << ", " << resourceInfo.sizeZ << "]"
               << ", attempted to access offset [" << offsetX << ", " << offsetY << ", " << offsetZ << "]"
               << ", remaining resource size being [" << remainingSizeX << ", " << remainingSizeY << ", " << remainingSizeZ << "]";
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::OutOfBounds;
            return false;
        }

        if (mipLevel >= resourceInfo.mipLevels)
        {
            std::stringstream ss;
            ss << resourceTypeName << " in copy command accesses a mip that is out of bounds," 
               << " mipLevel is " << mipLevel << " and total mip size is " << resourceInfo.mipLevels;
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::OutOfBounds;
            return false;
        }

        return true;
    };

    if (cmd->fullCopy)
    {
        const auto srcIt = context.resourceInfos->find(cmd->source);
        const auto dstIt = context.resourceInfos->find(cmd->destination);
        if (srcIt == context.resourceInfos->end() || dstIt == context.resourceInfos->end())
        {
            std::stringstream ss;
            ss << "Invalid resource passed on copy command";
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::InvalidResource;
            return false;
        }

        if (srcIt->second.sizeX != dstIt->second.sizeX 
        || srcIt->second.sizeY != dstIt->second.sizeY
        || srcIt->second.sizeZ != dstIt->second.sizeZ)
        {
            std::stringstream ss;
            ss << "Cannot copy resources, mismatching sizes.";
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::InvalidResource;
            return false;
        }
    }
    else
    {
        if (!fitsInCopyCmd(cmd->source, "Source", cmd->sourceX, cmd->sourceY, cmd->sourceZ, cmd->srcMipLevel))
            return false;

        if (!fitsInCopyCmd(cmd->destination, "Destination", cmd->destX, cmd->destY, cmd->destZ, cmd->dstMipLevel))
            return false;
    }

    return true;
}

bool processUpload(const AbiUploadCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    if (!transitionResource(cmd->destination, ResourceGpuState::CopyDst, context))
        return false;

    IDevice& device = *context.device;

    const auto resourceInfoIt = context.resourceInfos->find(cmd->destination);
    const WorkResourceInfo& resourceInfo = resourceInfoIt->second;

    CommandInfo& info = context.currentCommandInfo();
    device.getResourceMemoryInfo(cmd->destination, info.uploadDestinationMemoryInfo);

    if (info.uploadDestinationMemoryInfo.isBuffer)
    {
        size_t remainingSize = info.uploadDestinationMemoryInfo.byteSize - cmd->destX;
        if (cmd->sourceSize > remainingSize)
        {
            std::stringstream ss;
            ss << "Upload command request exceeded resource capacity. Tried to upload "
                << cmd->sourceSize << " bytes with a resource of "
                << info.uploadDestinationMemoryInfo.byteSize << "bytes." 
                << "offset being " << cmd->destX << " and remaining size being " << remainingSize;

            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::OutOfBounds;
            return false;
        }

        info.uploadBufferOffset = context.totalUploadBufferSize;
        context.totalUploadBufferSize += (int)remainingSize;
    }
    else
    {
        int szX = cmd->sizeX < 0 ? (info.uploadDestinationMemoryInfo.width  - cmd->destX) : cmd->sizeX;
        int szY = cmd->sizeY < 0 ? (info.uploadDestinationMemoryInfo.height - cmd->destY) : cmd->sizeY;
        int szZ = cmd->sizeZ < 0 ? (info.uploadDestinationMemoryInfo.depth  - cmd->destZ) : cmd->sizeZ;
        int srcBoxByteSize = szX * szY * szZ * info.uploadDestinationMemoryInfo.texelElementPitch;
        if (srcBoxByteSize > cmd->sourceSize)
        {
            std::stringstream ss;
            ss << "Upload command request exceed the source buffer capacity of "
                << cmd->sourceSize << "bytes. The box source size being "
                << szX << ", " << szY << ", " << szZ << "with a pixel pitch of " << info.uploadDestinationMemoryInfo.texelElementPitch 
                << "amounts to a total size of " << srcBoxByteSize;
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::OutOfBounds;
            return false;
        }

        int sizeLeftX = info.uploadDestinationMemoryInfo.width - cmd->destX;
        int sizeLeftY = info.uploadDestinationMemoryInfo.height  - cmd->destY;
        int sizeLeftZ = info.uploadDestinationMemoryInfo.depth - cmd->destZ;
        if (szX > sizeLeftX || szY > sizeLeftY || szZ > sizeLeftZ)
        {
            std::stringstream ss;
            ss << "Upload of texture window of size"
                << szX << ", " << szY << ", " << szZ 
                << " Exceeds the destination remaining size of "
                << sizeLeftX << ", " << sizeLeftY << ", ", sizeLeftZ;
            context.errorMsg = ss.str();
            context.errorType = ScheduleErrorType::OutOfBounds;
            return false;
        }

        int hardwareUploadSize = info.uploadDestinationMemoryInfo.rowPitch * szY * szZ; 
        info.uploadBufferOffset = context.totalUploadBufferSize;
        context.totalUploadBufferSize += hardwareUploadSize;
    }

    return true;
}

bool processDownload(const AbiDownloadCmd* cmd, const unsigned char* data, WorkBuildContext& context)
{
    const auto& resourceInfos = *context.resourceInfos;
    auto& resourceInfoIt = resourceInfos.find(cmd->source);
    if (resourceInfoIt == resourceInfos.end())
    {
        std::stringstream ss;
        ss << "Could not find resource with ID: " << cmd->source.handleId;
        context.errorType = ScheduleErrorType::InvalidResource;
        context.errorMsg = ss.str();
        return false;
    }

    if (cmd->mipLevel >= resourceInfoIt->second.mipLevels)
    {
        context.errorType = ScheduleErrorType::OutOfBounds;
        context.errorMsg = "Mip level for download out of bounds. Must be within range of resource";
        return false;
    }

    if (cmd->arraySlice >= resourceInfoIt->second.arraySlices)
    {
        context.errorType = ScheduleErrorType::OutOfBounds;
        context.errorMsg = "Array slice out of bounds. Must be within range of array slices";
        return false;
    }

    ResourceDownloadKey downloadKey { cmd->source, cmd->mipLevel, cmd->arraySlice };

    auto it = context.resourcesToDownload.insert(downloadKey);
    if (!it.second)
    {
        context.errorType = ScheduleErrorType::MultipleDownloadsOnSameResource;
        context.errorMsg = "Multiple downloads on the same resource during the same schedule call. You are only allowed to download a resource once per scheduling bundle.";
        return false;
    }

    if (!transitionResource(cmd->source, ResourceGpuState::CopySrc, context))
        return false;

    context.currentCommandInfo().commandDownloadIndex = context.currentListInfo().downloadCommandsCount;
    ++context.currentListInfo().downloadCommandsCount;

    return true;
}

bool processClearAppendConsume(const AbiClearAppendConsumeCounter* cmd, const unsigned char* data, WorkBuildContext& context)
{
    const auto& resourceInfos = *context.resourceInfos;
    auto& resourceInfoIt = resourceInfos.find(cmd->source);
    if (resourceInfoIt == resourceInfos.end())
    {
        std::stringstream ss;
        ss << "Could not find resource with ID: " << cmd->source.handleId;
        context.errorType = ScheduleErrorType::InvalidResource;
        context.errorMsg = ss.str();
        return false;
    }

    if (!resourceInfoIt->second.counterBuffer.valid())
    {
        std::stringstream ss;
        ss << "Resource in command clearAppendConsume is not an append consume buffer. Ensure this resource is a buffer with the flag isAppendConsume.";
        context.errorType = ScheduleErrorType::InvalidResource;
        context.errorMsg = ss.str();
        return false;
    }

    if (!transitionResource(resourceInfoIt->second.counterBuffer, ResourceGpuState::CopyDst, context))
        return false;

    CommandInfo& info = context.currentCommandInfo();
    info.uploadBufferOffset = context.totalUploadBufferSize;
    context.totalUploadBufferSize += 4u; //we are gonna just copy one int.
    return true;
}

void parseCommandList(const unsigned char* data, WorkBuildContext& context)
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
                {
                    const auto* abiCmd = (const AbiComputeCmd*)(data + offset);
                    finished = !processCompute(abiCmd, data, context);
                    offset += abiCmd->cmdSize;
                }
                break;
            case AbiCmdTypes::Copy:
                {
                    const auto* abiCmd = (const AbiCopyCmd*)(data + offset);
                    finished = !processCopy(abiCmd, data, context);
                    offset += abiCmd->cmdSize;
                }
                break;
            case AbiCmdTypes::Upload:
                {
                    const auto* abiCmd = (const AbiUploadCmd*)(data + offset);
                    finished = !processUpload(abiCmd, data, context);
                    offset += abiCmd->cmdSize;
                }
                break;
            case AbiCmdTypes::Download:
                {
                    const auto* abiCmd = (const AbiDownloadCmd*)(data + offset);
                    finished = !processDownload(abiCmd, data, context);
                    offset += abiCmd->cmdSize;
                }
                break;
            case AbiCmdTypes::ClearAppendConsumeCounter:
                {
                    const auto* abiCmd = (const AbiClearAppendConsumeCounter*)(data + offset);
                    finished = !processClearAppendConsume(abiCmd, data, context);
                    offset += abiCmd->cmdSize;
                }
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
        //todo error handling
        WorkBuildContext ctx;
        ctx.device = &m_device;
        ctx.resourceInfos = &m_resources;
        ctx.tableInfos = &m_tables;
        for (int l = 0; l < listCount; ++l)
        {
            CommandList* list = lists[l];
            if (!list)
            {
                std::stringstream ss;
                ss << "List at index " << l << " is a null pointer.";
                ctx.errorType = ScheduleErrorType::NullListFound;
                ctx.errorMsg = ss.str();
                break;
            }

            if (!list->isFinalized())
            {
                std::stringstream ss;
                ss << "List at index " << l << " not finalized.";
                ctx.errorType = ScheduleErrorType::ListNotFinalized;
                ctx.errorMsg = ss.str();
                break;
            }

            ctx.listIndex = l;
            ctx.currentCommandIndex = 0;
            ctx.command = 0;
            ctx.processedList.emplace_back();
            parseCommandList(list->data(), ctx);
        }

        if (ctx.errorType != ScheduleErrorType::Ok)
            return ScheduleStatus { handle, ctx.errorType, std::move(ctx.errorMsg) };

        auto& workData = m_works.allocate(handle);
        workData.processedLists = std::move(ctx.processedList);
        workData.states = std::move(ctx.states);
        workData.tableAllocations = std::move(ctx.tableAllocations);
        workData.resourcesToDownload = std::move(ctx.resourcesToDownload);
        workData.totalTableSize = ctx.totalTableSize;
        workData.totalConstantBuffers = ctx.totalConstantBuffers;
        workData.totalUploadBufferSize = ctx.totalUploadBufferSize;
        workData.totalSamplers = ctx.totalSamplers;
    }

    return ScheduleStatus { handle, ScheduleErrorType::Ok, "" };
}

bool WorkBundleDb::writeResourceStates(WorkHandle handle)
{
    std::unique_lock lock(m_workMutex);
    CPY_ASSERT(handle.valid());
    CPY_ASSERT(m_works.contains(handle));
    if (!handle.valid() || !m_works.contains(handle))
        return false;

    auto& bundle = m_works[handle];
    return commitResourceStates(bundle.states, m_resources);
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

void WorkBundleDb::registerTable(ResourceTable table, const char* name, const ResourceHandle* handles, int handleCounts, bool isUav)
{
    auto& newInfo = m_tables[table];
    newInfo.name = name;
    newInfo.isUav = isUav;
    newInfo.resources.assign(handles, handles + handleCounts);
}

void WorkBundleDb::unregisterTable(ResourceTable table)
{
    m_tables.erase(table);
}

void WorkBundleDb::registerResource(
    ResourceHandle handle,
    MemFlags flags,
    ResourceGpuState initialState,
    int sizeX, int sizeY, int sizeZ,
    int mipLevels,
    int arraySlices,
    Buffer counterBuffer)
{
    auto& resInfo = m_resources[handle];
    resInfo.memFlags = flags;
    resInfo.gpuState = initialState;
    resInfo.counterBuffer = counterBuffer;
    resInfo.sizeX = sizeX;
    resInfo.sizeY = sizeY;
    resInfo.sizeZ = sizeZ;
    resInfo.mipLevels = mipLevels;
    resInfo.arraySlices = arraySlices;
}

void WorkBundleDb::unregisterResource(ResourceHandle handle)
{
    m_resources.erase(handle);
}

}

}
