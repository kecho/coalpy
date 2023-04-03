#include "VulkanWorkBundle.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanEventPool.h"
#include "VulkanShaderDb.h"
#include "VulkanBarriers.h"
#include "VulkanUtils.h"
#include "VulkanFormats.h"
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
    VulkanShaderDb& db = (VulkanShaderDb&)*m_device.db();
    db.resolve(computeCmd->shader);
    const SpirvPayload& shaderPayload = db.unsafeGetSpirvPayload(computeCmd->shader);

    std::vector<VkDescriptorSet> sets;
    sets.reserve(shaderPayload.descriptorSetsInfos.size());
    for (const VulkanDescriptorSetInfo& setInfo : shaderPayload.descriptorSetsInfos)
        sets.push_back(m_descriptors->allocateDescriptorSet(setInfo.layout));

    std::vector<VkCopyDescriptorSet> copies;
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> cbuffers;
    VulkanResources& resources = m_device.resources();
    auto fillDescriptors = [&copies, &sets, &resources, &shaderPayload]
        (SpirvRegisterType type, int tableCounts, const ResourceTable* tables)
    {
        for (int i = 0; i < tableCounts; ++i)
        {
            if (i >= sets.size())
                break;

            uint64_t activeMask = shaderPayload.activeDescriptors[i][(int)type];
            while (activeMask)
            {
                ResourceTable tableHandle = tables[i];
                const VulkanResourceTable& table = resources.unsafeGetTable(tableHandle);
                int startBinding, bindingCounts;                
                SpirvPayload::nextDescriptorRange(activeMask, startBinding, bindingCounts);
                if (startBinding >= (int)table.counts)
                    continue;

                copies.emplace_back();
                VkCopyDescriptorSet& copy = copies.back();
                copy = { VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, nullptr };
                copy.srcSet = table.descriptors.descriptors;
                copy.srcBinding = startBinding;
                copy.dstSet = sets[i];
                copy.dstBinding = (uint32_t)SpirvRegisterTypeOffset(type) + startBinding;
                copy.descriptorCount = std::min(bindingCounts, (int)table.counts);
            }
        }
    };

    if (computeCmd->inlineConstantBufferSize > 0)
    {
        writes.emplace_back();
        VkWriteDescriptorSet& write = writes.back();
        write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr };

        cbuffers.emplace_back();
        VkDescriptorBufferInfo& cbuffer = cbuffers.back();
        
        CPY_ASSERT(computeCmd->inlineConstantBufferSize <= (m_uploadMemBlock.uploadSize - cmdInfo.uploadBufferOffset));
        memcpy((char*)m_uploadMemBlock.mappedBuffer + cmdInfo.uploadBufferOffset, computeCmd->inlineConstantBuffer.data(data), computeCmd->inlineConstantBufferSize);

        unsigned alignedSize = alignByte((unsigned)computeCmd->inlineConstantBufferSize, (unsigned)ConstantBufferAlignment);
        write.dstSet = sets[0]; //always write to set 0
        write.dstBinding = (uint32_t)SpirvRegisterTypeOffset(SpirvRegisterType::b);
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &cbuffer;
        
        cbuffer.buffer = resources.unsafeGetResource(m_uploadMemBlock.buffer).bufferData.vkBuffer;
        cbuffer.offset = cmdInfo.uploadBufferOffset;
        cbuffer.range = alignedSize;
    }
    else if (computeCmd->constantCounts > 0)
    {
        cbuffers.resize(computeCmd->constantCounts);
        for (int i = 0; i < computeCmd->constantCounts; ++i)
        {
            writes.emplace_back();
            const Buffer& constantBuffer = *(computeCmd->constants.data(data) + i);
            VkWriteDescriptorSet& write = writes[i];
            VkDescriptorBufferInfo& cbuffer = cbuffers[i];

            write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr };
            write.dstSet = sets[i];
            write.dstBinding = (uint32_t)SpirvRegisterTypeOffset(SpirvRegisterType::b);
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &cbuffer;

            auto& bufferData = resources.unsafeGetResource(constantBuffer).bufferData;
            cbuffer.buffer = bufferData.vkBuffer;
            cbuffer.offset = 0;
            cbuffer.range = bufferData.size;
        }
    }

    fillDescriptors(SpirvRegisterType::t, computeCmd->inResourceTablesCounts, (const ResourceTable*)computeCmd->inResourceTables.data(data));
    fillDescriptors(SpirvRegisterType::u, computeCmd->outResourceTablesCounts, (const ResourceTable*)computeCmd->outResourceTables.data(data));
    fillDescriptors(SpirvRegisterType::s, computeCmd->samplerTablesCounts, (const ResourceTable*)computeCmd->samplerTables.data(data));
    vkUpdateDescriptorSets(m_device.vkDevice(), writes.size(), writes.data(), copies.size(), copies.data());

    VkCommandBuffer cmdBuffer = outList.list;
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, shaderPayload.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, shaderPayload.pipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);
    if (computeCmd->isIndirect)
    {
        VkBuffer argBuffer = resources.unsafeGetResource(computeCmd->indirectArguments).bufferData.vkBuffer;
        vkCmdDispatchIndirect(cmdBuffer, argBuffer, 0u);
    }
    else
        vkCmdDispatch(cmdBuffer, computeCmd->x, computeCmd->y, computeCmd->z);
}

void VulkanWorkBundle::buildUploadCmd(const unsigned char* data, const AbiUploadCmd* uploadCmd, const CommandInfo& cmdInfo, VulkanList& outList)
{
    VulkanResources& resources = m_device.resources();
    VulkanResource& destinationResource = resources.unsafeGetResource(uploadCmd->destination);
    CPY_ASSERT_FMT(cmdInfo.uploadBufferOffset < m_uploadMemBlock.uploadSize, "out of bounds offset: %d < %d", cmdInfo.uploadBufferOffset, m_uploadMemBlock.uploadSize);
    CPY_ASSERT((m_uploadMemBlock.uploadSize - cmdInfo.uploadBufferOffset) >= uploadCmd->sourceSize);
    VkBuffer srcBuffer = resources.unsafeGetResource(m_uploadMemBlock.buffer).bufferData.vkBuffer;
    if (destinationResource.isBuffer())
    {
        memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + cmdInfo.uploadBufferOffset, uploadCmd->sources.data(data), uploadCmd->sourceSize);
        VkBufferCopy region = { (VkDeviceSize)(m_uploadMemBlock.offset + cmdInfo.uploadBufferOffset), (VkDeviceSize)uploadCmd->destX, (VkDeviceSize)uploadCmd->sourceSize };
        vkCmdCopyBuffer(outList.list, srcBuffer, destinationResource.bufferData.vkBuffer, 1, &region);
    }
    else
    {
        int formatStride = getVkFormatStride(destinationResource.textureData.format);
        CPY_ASSERT(formatStride == cmdInfo.uploadDestinationMemoryInfo.texelElementPitch);
        auto isArray = [](TextureType t) { return t == TextureType::k2dArray || t == TextureType::CubeMapArray || t == TextureType::CubeMap; };

        int szX = uploadCmd->sizeX < 0 ? (cmdInfo.uploadDestinationMemoryInfo.width  - uploadCmd->destX) : uploadCmd->sizeX;
        int szY = uploadCmd->sizeY < 0 ? (cmdInfo.uploadDestinationMemoryInfo.height - uploadCmd->destY) : uploadCmd->sizeY;
        int szZ = uploadCmd->sizeZ < 0 ? (cmdInfo.uploadDestinationMemoryInfo.depth  - uploadCmd->destZ) : uploadCmd->sizeZ;
        int segments = szY * szZ;
        int sourceRowPitch = szX * formatStride;
        memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + cmdInfo.uploadBufferOffset, uploadCmd->sources.data(data), sourceRowPitch * segments);
        
        VkBufferImageCopy region = {};
        region.bufferOffset = m_uploadMemBlock.offset + cmdInfo.uploadBufferOffset;
        region.bufferRowLength = 0; //tightly packed
        region.bufferImageHeight = 0; //tightly packed
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = uploadCmd->mipLevel;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = isArray ? szZ : 1;
        region.imageOffset.x = uploadCmd->destX;
        region.imageOffset.y = uploadCmd->destY;
        region.imageOffset.z = uploadCmd->destZ;
        region.imageExtent.width = szX;
        region.imageExtent.height = szY;
        region.imageExtent.depth = szZ;
        vkCmdCopyBufferToImage(outList.list, srcBuffer, destinationResource.textureData.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
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
            sizeToCopy = src.requestSize;
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
                int dstRemainingSize = ((int)dst.requestSize - copyCmd->destX);
                int srcRemainingSize = ((int)src.requestSize - copyCmd->sourceX);
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
        CPY_ASSERT(src.isTexture());
        CPY_ASSERT(dst.isTexture());
        VkImage srcImage = src.textureData.vkImage;
        VkImage dstImage = dst.textureData.vkImage;

        auto zAsSlice = [](TextureType t) { return t == TextureType::k2dArray || t == TextureType::CubeMapArray || t == TextureType::CubeMap; };
        const bool srcZAsSlice = zAsSlice(src.textureData.textureType);
        int srcSliceId = srcZAsSlice ? copyCmd->sourceZ : 0;
        int dstSliceId = zAsSlice(dst.textureData.textureType) ? copyCmd->destZ : 0;

        VkImageCopy region = {};
        
        int szX = copyCmd->sizeX < 0 ? (std::min(src.textureData.width  - copyCmd->sourceX, src.textureData.width   - copyCmd->destX)) : copyCmd->sizeX;
        int szY = copyCmd->sizeY < 0 ? (std::min(src.textureData.height - copyCmd->sourceY, src.textureData.height  - copyCmd->destY)) : copyCmd->sizeY;
        int szZ = copyCmd->sizeZ < 0 ? (std::min(src.textureData.depth  - copyCmd->sourceZ, src.textureData.depth   - copyCmd->destZ)) : copyCmd->sizeZ;
        region.srcOffset = VkOffset3D { copyCmd->sourceX, copyCmd->sourceY, srcSliceId };
        region.srcSubresource = VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)copyCmd->srcMipLevel, (uint32_t)srcSliceId, 1u };
        region.dstOffset = VkOffset3D { copyCmd->destX, copyCmd->destX, dstSliceId };
        region.dstSubresource = VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)copyCmd->dstMipLevel, (uint32_t)dstSliceId, 1u };
        region.extent = VkExtent3D { (uint32_t)szX, (uint32_t)szY, srcZAsSlice ? (uint32_t)szZ : 1u };

        vkCmdCopyImage(
            outList.list,
            srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
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
    downloadState.requestedSize = resource.requestSize;
    VkBuffer dstBuffer = resources.unsafeGetResource(downloadState.memoryBlock.buffer).bufferData.vkBuffer;
    if (resource.isBuffer())
    {
        VkBuffer srcBuffer = resource.bufferData.vkBuffer;
        VkBufferCopy region = { 0ull, downloadState.memoryBlock.offset, resource.requestSize };
        vkCmdCopyBuffer(outList.list, srcBuffer, dstBuffer, 1, &region);
    }
    else
    {
        VulkanResource::TextureData& textureData = resource.textureData;
        downloadState.width = (textureData.width >> downloadCmd->mipLevel);
        downloadState.height = (textureData.height >> downloadCmd->mipLevel);
        downloadState.depth = (textureData.textureType == TextureType::k3d) ? (textureData.depth >> downloadCmd->mipLevel) : 1;
        downloadState.rowPitch = downloadState.width * getVkFormatStride(textureData.format);
        VkImage srcImage = textureData.vkImage;
        VkBufferImageCopy region = {};
        region.bufferOffset = downloadState.memoryBlock.offset;
        region.bufferRowLength = 0; //tightly packed
        region.bufferImageHeight = 0; //tightly packed
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = downloadCmd->mipLevel;
        region.imageSubresource.baseArrayLayer = downloadCmd->arraySlice;
        region.imageSubresource.layerCount = 1;
        region.imageOffset.x = 0; 
        region.imageOffset.y = 0; 
        region.imageOffset.z = 0; 
        region.imageExtent.width = downloadState.width;
        region.imageExtent.height = downloadState.height;
        region.imageExtent.depth = downloadState.depth;
        vkCmdCopyImageToBuffer(outList.list, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer, 1, &region);
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
        EventState postEventState = createSrcBarrierEvent(m_device, m_device.eventPool(), cmdInfo.postBarrier.data(), cmdInfo.postBarrier.size(), outList.list);
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
        applyBarriers(m_device, s_nullEvent, m_device.eventPool(), cmdInfo.preBarrier.data(), (int)cmdInfo.preBarrier.size(), outList.list);
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
        applyBarriers(m_device, postEventState, m_device.eventPool(), cmdInfo.postBarrier.data(), (int)cmdInfo.postBarrier.size(), outList.list);
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
