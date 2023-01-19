#include "VulkanWorkBundle.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanEventPool.h"
#include "VulkanShaderDb.h"
#include "VulkanBarriers.h"
#include <coalpy.core/Assert.h>
#include <vector>
#include <unordered_map>
#include <string.h>
#include <iostream>

#define DEBUG_EXECUTION 0

namespace coalpy
{
namespace render
{

bool VulkanWorkBundle::load(const WorkBundle& workBundle)
{
    m_workBundle = workBundle;
    return true;
}

void VulkanWorkBundle::buildComputeCmd(const unsigned char* data, const AbiComputeCmd* computeCmd, const CommandInfo& cmdInfo, VulkanList& outList)
{
    const VulkanShaderDb& shaderDb = (VulkanShaderDb&)*m_device.db();
    const SpirvPayload& shaderPayload = shaderDb.unsafeGetSpirvPayload(computeCmd->shader);

    std::vector<VkDescriptorSet> sets;
    sets.reserve(shaderPayload.descriptorSetsInfos.size());
    for (const VulkanDescriptorSetInfo& setInfo : shaderPayload.descriptorSetsInfos)
        sets.push_back(m_descriptors->allocateDescriptorSet(setInfo.layout));

    std::vector<VkCopyDescriptorSet> copies;
    VulkanResources& resources = m_device.resources();
    auto fillDescriptors = [&copies, &sets, &resources](SpirvRegisterType type, int tableCounts, const ResourceTable* tables)
    {
        for (int i = 0; i < tableCounts; ++i)
        {
            if (i >= sets.size())
                break;

            ResourceTable tableHandle = tables[i];
            const VulkanResourceTable& table = resources.unsafeGetTable(tableHandle);
            copies.emplace_back();
            VkCopyDescriptorSet& copy = copies.back();
            copy = { VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, nullptr };
            copy.srcSet = table.descriptors.descriptors;
            copy.srcBinding = 0;
            copy.dstSet = sets[i];
            copy.dstBinding = SpirvRegisterTypeOffset(type);
            copy.descriptorCount = table.counts;
        }
    };

    fillDescriptors(SpirvRegisterType::t, computeCmd->inResourceTablesCounts, (const ResourceTable*)computeCmd->inResourceTables.data(data));
    fillDescriptors(SpirvRegisterType::u, computeCmd->outResourceTablesCounts, (const ResourceTable*)computeCmd->outResourceTables.data(data));
    vkUpdateDescriptorSets(m_device.vkDevice(), 0, nullptr, copies.size(), copies.data());

    VkCommandBuffer cmdBuffer = outList.list;
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, shaderPayload.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, shaderPayload.pipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);
    if (!computeCmd->isIndirect)
        vkCmdDispatch(cmdBuffer, computeCmd->x, computeCmd->y, computeCmd->z);
    else
    {
        CPY_ASSERT_FMT(false, "%s", "Unimplemented");
    }
}

void VulkanWorkBundle::buildUploadCmd(const unsigned char* data, const AbiUploadCmd* uploadCmd, const CommandInfo& cmdInfo, VulkanList& outList)
{
    VulkanResources& resources = m_device.resources();
    VulkanResource& destinationResource = resources.unsafeGetResource(uploadCmd->destination);
    CPY_ASSERT_FMT(cmdInfo.uploadBufferOffset < m_uploadMemBlock.uploadSize, "out of bounds offset: %d < %d", cmdInfo.uploadBufferOffset, m_uploadMemBlock.uploadSize);
    CPY_ASSERT((m_uploadMemBlock.uploadSize - cmdInfo.uploadBufferOffset) >= uploadCmd->sourceSize);
    if (destinationResource.isBuffer())
    {
        memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + cmdInfo.uploadBufferOffset, uploadCmd->sources.data(data), uploadCmd->sourceSize);
        VkBufferCopy region = { (VkDeviceSize)(m_uploadMemBlock.offset + cmdInfo.uploadBufferOffset), (VkDeviceSize)uploadCmd->destX, (VkDeviceSize)uploadCmd->sourceSize };
        VkBuffer srcBuffer = resources.unsafeGetResource(m_uploadMemBlock.buffer).bufferData.vkBuffer;
        vkCmdCopyBuffer(outList.list, srcBuffer, destinationResource.bufferData.vkBuffer, 1, &region);
    }
    else
    {
        CPY_ASSERT_FMT(false, "%s", "unimplemented");
    }
    
}

void VulkanWorkBundle::buildCopyCmd(const unsigned char* data, const AbiCopyCmd* copyCmd, const CommandInfo& cmdInfo, VulkanList& outList)
{
    VulkanResources& resources = m_device.resources();
    VulkanResource& src = resources.unsafeGetResource(copyCmd->source);
    VulkanResource& dst = resources.unsafeGetResource(copyCmd->destination);
    if (src.isBuffer())
    {
        uint64_t sizeToCopy =  0ull;
        uint64_t srcOffset  =  0ull;
        uint64_t destOffset =  0ull;
        if (copyCmd->fullCopy)
        {
            sizeToCopy = src.actualSize;
        }
        else
        {
            srcOffset = copyCmd->sourceX;
            destOffset = copyCmd->destX;
            if (copyCmd->sizeX >= 0)
            {
                sizeToCopy = copyCmd->sizeX;
            }
            else
            {
                int dstRemainingSize = ((int)dst.actualSize - copyCmd->destX);
                int srcRemainingSize = ((int)src.actualSize - copyCmd->sourceX);
                sizeToCopy = std::min(dstRemainingSize, srcRemainingSize);
            }
        }
        VkBufferCopy region = { srcOffset, destOffset, sizeToCopy };
        VkBuffer srcBuffer = src.bufferData.vkBuffer;
        VkBuffer dstBuffer = dst.bufferData.vkBuffer;
        vkCmdCopyBuffer(outList.list, srcBuffer, dstBuffer, 1, &region);
    }
    else
    {
        CPY_ASSERT_FMT(false, "%s", "unimplemented");
    }
}

void VulkanWorkBundle::buildDownloadCmd(
    const unsigned char* data, const AbiDownloadCmd* downloadCmd,
    const CommandInfo& cmdInfo, VulkanFenceHandle fenceValue, VulkanList& outList)
{
    VulkanResources& resources = m_device.resources();
    CPY_ASSERT(cmdInfo.commandDownloadIndex >= 0 && cmdInfo.commandDownloadIndex < (int)m_downloadStates.size());
    CPY_ASSERT(downloadCmd->source.valid());
    VulkanResource& resource = resources.unsafeGetResource(downloadCmd->source);
    VulkanResourceDownloadState& downloadState = m_downloadStates[cmdInfo.commandDownloadIndex];
    downloadState.downloadKey = ResourceDownloadKey { downloadCmd->source, downloadCmd->mipLevel, downloadCmd->arraySlice };
    downloadState.memoryBlock = m_device.readbackPool().allocate(resource.actualSize);
    VkBuffer dstBuffer = resources.unsafeGetResource(downloadState.memoryBlock.buffer).bufferData.vkBuffer;
    if (resource.isBuffer())
    {
        VkBuffer srcBuffer = resource.bufferData.vkBuffer;

        VkBufferCopy region = { 0ull, downloadState.memoryBlock.offset, resource.actualSize };
        vkCmdCopyBuffer(outList.list, srcBuffer, dstBuffer, 1, &region);
    }
    else
    {
        CPY_ASSERT_FMT(false, "%s", "unimplemented");
    }

    downloadState.queueType = WorkType::Graphics;
    downloadState.fenceValue = fenceValue;
    downloadState.resource = downloadCmd->source;
    m_device.fencePool().addRef(fenceValue);
}

void VulkanWorkBundle::buildCommandList(int listIndex, const CommandList* cmdList, WorkType workType, VulkanList& outList, std::vector<VulkanEventHandle>& events, VulkanFenceHandle fenceValue)
{
    CPY_ASSERT(cmdList->isFinalized());
    const unsigned char* listData = cmdList->data();
    const ProcessedList& pl = m_workBundle.processedLists[listIndex];
    if (!pl.commandSchedule.empty())
    {
        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(outList.list, &beginInfo);
    }
    
    for (int commandIndex = 0; commandIndex < pl.commandSchedule.size(); ++commandIndex)
    {
        const CommandInfo& cmdInfo = pl.commandSchedule[commandIndex];
        const unsigned char* cmdBlob = listData + cmdInfo.commandOffset;
        AbiCmdTypes cmdType = *((AbiCmdTypes*)cmdBlob);
        EventState postEventState = createSrcBarrierEvent(m_device, m_device.eventPool(), cmdInfo.postBarrier, outList.list);
        #if DEBUG_EXECUTION
            if (postEventState.eventHandle.valid())
                std::cout << "[CmdBuffer] Src Event begin" << std::endl;
        #endif
        if (postEventState.eventHandle.valid())
            events.emplace_back(postEventState.eventHandle);
        static const EventState s_nullEvent; 
        #if DEBUG_EXECUTION
        std::cout << "[CmdBuffer] Pre apply barriers" << std::endl;
        #endif
        applyBarriers(m_device, s_nullEvent, m_device.eventPool(), cmdInfo.preBarrier, outList.list);
        switch (cmdType)
        {
        case AbiCmdTypes::Compute:
            {
                const auto* abiCmd = (const AbiComputeCmd*)cmdBlob;
                buildComputeCmd(listData, abiCmd, cmdInfo, outList);
            }
            break;
        case AbiCmdTypes::Copy:
            {
                const auto* abiCmd = (const AbiCopyCmd*)cmdBlob;
                buildCopyCmd(listData, abiCmd, cmdInfo, outList);
            }
            break;
        case AbiCmdTypes::Upload:
            {
                const auto* abiCmd = (const AbiUploadCmd*)cmdBlob;
                buildUploadCmd(listData, abiCmd, cmdInfo, outList);
            }
            break;
        case AbiCmdTypes::Download:
            {
                const auto* abiCmd = (const AbiDownloadCmd*)cmdBlob;
                buildDownloadCmd(listData, abiCmd, cmdInfo, fenceValue, outList);
            }
            break;
        case AbiCmdTypes::CopyAppendConsumeCounter:
            {
                //const auto* abiCmd = (const AbiCopyAppendConsumeCounter*)cmdBlob;
                //buildCopyAppendConsumeCounter(listData, abiCmd, cmdInfo, outList);
            }
            break;
        case AbiCmdTypes::ClearAppendConsumeCounter:
            {
                //const auto* abiCmd = (const AbiClearAppendConsumeCounter*)cmdBlob;
                //buildClearAppendConsumeCounter(listData, abiCmd, cmdInfo, outList);
            }
            break;
        case AbiCmdTypes::BeginMarker:
            {
                /*
                const auto* abiCmd = (const AbiBeginMarker*)cmdBlob;
                Dx12PixApi* pixApi = m_device.getPixApi();
                if (pixApi)
                {
                    const char* str = abiCmd->str.data(listData);
                    pixApi->pixBeginEventOnCommandList(&outList, 0xffff00ff, str);
                }

                Dx12MarkerCollector& markerCollector = m_device.markerCollector();
                if (markerCollector.isActive())
                {
                    const char* str = abiCmd->str.data(listData);
                    markerCollector.beginMarker(outList, str);
                }
                */
            }
            break;
        case AbiCmdTypes::EndMarker:
            {
                /*
                Dx12PixApi* pixApi = m_device.getPixApi();
                if (pixApi)
                    pixApi->pixEndEventOnCommandList(&outList);

                Dx12MarkerCollector& markerCollector = m_device.markerCollector();
                if (markerCollector.isActive())
                    markerCollector.endMarker(outList);
                */
            }
            break;
        default:
            CPY_ASSERT_FMT(false, "Unrecognized serialized command %d", cmdType);
            return;
        }
        #if DEBUG_EXECUTION
        std::cout << "[CmdBuffer] Post apply barriers" << std::endl;
        #endif
        applyBarriers(m_device, postEventState, m_device.eventPool(), cmdInfo.postBarrier, outList.list);
    }

    if (!pl.commandSchedule.empty())
        vkEndCommandBuffer(outList.list);
}

VulkanFenceHandle VulkanWorkBundle::execute(CommandList** commandLists, int commandListsCount)
{
    
    CPY_ASSERT(commandListsCount == (int)m_workBundle.processedLists.size());
    WorkType workType = WorkType::Graphics;
    VulkanQueues& queues = m_device.queues();    
    queues.syncFences(workType);

    VkQueue queue = queues.cmdQueue(workType);
    VulkanMemoryPools& pools = queues.memPools(workType);
    VulkanFenceHandle fenceHandle = queues.newFence();
    VkFence fence = m_device.fencePool().get(fenceHandle);
    pools.uploadPool->beginUsage(fenceHandle);
    pools.descriptors->beginUsage(fenceHandle);
    m_descriptors = pools.descriptors;
    m_downloadStates.resize((int)m_workBundle.resourcesToDownload.size());

    if (m_workBundle.totalUploadBufferSize)
        m_uploadMemBlock = pools.uploadPool->allocUploadBlock(m_workBundle.totalUploadBufferSize);

    std::vector<VulkanList> lists;
    std::vector<VkCommandBuffer> cmdBuffers;
    std::vector<VulkanEventHandle> events;
    for (int i = 0; i < commandListsCount; ++i)
    {
        lists.emplace_back();
        VulkanList& list = lists.back();
        queues.allocate(workType, list);
        cmdBuffers.push_back(list.list);

        buildCommandList(i, commandLists[i], workType, list, events, fenceHandle);
    }

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
    submitInfo.commandBufferCount = cmdBuffers.size();
    submitInfo.pCommandBuffers = cmdBuffers.data();

    VK_OK(vkQueueSubmit(queue, 1u, &submitInfo, fence));

    for (int i = 0; i < (int)lists.size(); ++i)
    {
        auto& l = lists[i];
        queues.deallocate(l, fenceHandle);
    }

    for (auto& eventHandle : events)
        m_device.eventPool().release(eventHandle);
    
    pools.descriptors->endUsage();
    pools.uploadPool->endUsage();    
    return fenceHandle;
}

void VulkanWorkBundle::getDownloadResourceMap(VulkanDownloadResourceMap& downloadMap)
{
    for (auto& state : m_downloadStates)
        downloadMap[state.downloadKey] = state;
}

}
}
