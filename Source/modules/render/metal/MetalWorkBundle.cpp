#include <Config.h>
#if ENABLE_METAL

#include <Metal/Metal.h>
#include "MetalDevice.h"
#include "MetalResources.h"
#include "MetalShaderDb.h"
#include "MetalQueues.h"
#include "MetalWorkBundle.h"

namespace coalpy
{
namespace render
{

static void buildComputeCmd(
    MetalDevice* device,
    const unsigned char* data,
    const AbiComputeCmd* computeCmd,
    const CommandInfo& cmdInfo,
    id<MTLCommandBuffer> cmdBuffer
)
{
    MetalShaderDb* db = &device->shaderDb();
    db->resolve(computeCmd->shader);
    MetalPayload* payload = db->getMetalPayload(computeCmd->shader);

    id<MTLComputeCommandEncoder> encoder = [cmdBuffer computeCommandEncoder];
    [encoder setComputePipelineState:payload->mtlPipelineState];

    MTLSize gridSize = MTLSizeMake(computeCmd->x, computeCmd->y, computeCmd->z);
    MTLSize threadgroupSize = MTLSizeMake(
        payload->threadGroupSizeX,
        payload->threadGroupSizeY,
        payload->threadGroupSizeZ
    );

    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
    [encoder endEncoding];
}

static void buildDownloadCmd(
    MetalDevice* device,
    std::vector<MetalResourceDownloadState>* downloadStates,
    const unsigned char* data,
    const AbiDownloadCmd* downloadCmd,
    const CommandInfo& cmdInfo,
    id<MTLCommandBuffer> cmdBuffer
)
{
    CPY_ASSERT(cmdInfo.commandDownloadIndex >= 0 && cmdInfo.commandDownloadIndex < (int)downloadStates->size());
    CPY_ASSERT(downloadCmd->source.valid());

    MetalResources& resources = device->resources();
    MetalResourceDownloadState* downloadState = &(*downloadStates)[cmdInfo.commandDownloadIndex];
    downloadState->downloadKey = ResourceDownloadKey { downloadCmd->source, downloadCmd->mipLevel, downloadCmd->arraySlice };
    MetalResource& metalResource = resources.unsafeGetResource(downloadCmd->source);
    if (metalResource.type == MetalResource::Type::Buffer)
    {
        downloadState->memoryBlock = device->readbackPool().allocate(metalResource.sizeInBytes);

    }
    else
    {
        CPY_ASSERT(metalResource.type == MetalResource::Type::Texture)
        MetalResource::TextureData& textureData = metalResource.textureData;
    }
}

bool MetalWorkBundle::load(const WorkBundle& workBundle)
{
    m_workBundle = workBundle;
    return true;
}

int MetalWorkBundle::execute(CommandList** cmdLists, int cmdListsCount)
{
    WorkType workType = WorkType::Graphics;
    auto& queues = m_device.queues();
    id<MTLCommandQueue> cmdQueue = queues.cmdQueue(workType);

    m_downloadStates.resize((int)m_workBundle.resourcesToDownload.size());

    id<MTLCommandBuffer> cmdBuffer = [cmdQueue commandBuffer];
    for (int i = 0; i < cmdListsCount; ++i)
    {
        CommandList* cmdList = cmdLists[i];
        CPY_ASSERT(cmdList->isFinalized());
        const unsigned char* listData = cmdList->data();
        const ProcessedList& pl = m_workBundle.processedLists[i];
        for (int j = 0; j < pl.commandSchedule.size(); ++j)
        {
            const CommandInfo& cmdInfo = pl.commandSchedule[j];
            // applyBarriers(cmdInfo.preBarrier, outList); // TODO

            const unsigned char* cmdBlob = listData + cmdInfo.commandOffset;
            AbiCmdTypes cmdType = *((AbiCmdTypes*)cmdBlob);
            switch (cmdType)
            {
            case AbiCmdTypes::Compute:
                {
                    const auto* abiCmd = (const AbiComputeCmd*)cmdBlob;
                    buildComputeCmd(&m_device, listData, abiCmd, cmdInfo, cmdBuffer);
                }
                break;
            // case AbiCmdTypes::Copy:
            //     {
            //         const auto* abiCmd = (const AbiCopyCmd*)cmdBlob;
            //         buildCopyCmd(listData, abiCmd, outList);
            //     }
            //     break;
            // case AbiCmdTypes::Upload:
            //     {
            //         const auto* abiCmd = (const AbiUploadCmd*)cmdBlob;
            //         buildUploadCmd(listData, abiCmd, cmdInfo, outList);
            //     }
            //     break;
            case AbiCmdTypes::Download:
                {
                    const auto* abiCmd = (const AbiDownloadCmd*)cmdBlob;
                    buildDownloadCmd(&m_device, &m_downloadStates, listData, abiCmd, cmdInfo, cmdBuffer);
                }
                break;
            // case AbiCmdTypes::CopyAppendConsumeCounter:
            //     {
            //         const auto* abiCmd = (const AbiCopyAppendConsumeCounter*)cmdBlob;
            //         buildCopyAppendConsumeCounter(listData, abiCmd, cmdInfo, outList);
            //     }
            //     break;
            // case AbiCmdTypes::ClearAppendConsumeCounter:
            //     {
            //         const auto* abiCmd = (const AbiClearAppendConsumeCounter*)cmdBlob;
            //         buildClearAppendConsumeCounter(listData, abiCmd, cmdInfo, outList);
            //     }
            //     break;
            // case AbiCmdTypes::BeginMarker:
            //     {
            //         const auto* abiCmd = (const AbiBeginMarker*)cmdBlob;
            //         Dx12PixApi* pixApi = m_device.getPixApi();
            //         if (pixApi)
            //         {
            //             const char* str = abiCmd->str.data(listData);
            //             pixApi->pixBeginEventOnCommandList(&outList, 0xffff00ff, str);
            //         }

            //         Dx12MarkerCollector& markerCollector = m_device.markerCollector();
            //         if (markerCollector.isActive())
            //         {
            //             const char* str = abiCmd->str.data(listData);
            //             markerCollector.beginMarker(outList, str);
            //         }
            //     }
            //     break;
            // case AbiCmdTypes::EndMarker:
            //     {
            //         Dx12PixApi* pixApi = m_device.getPixApi();
            //         if (pixApi)
            //             pixApi->pixEndEventOnCommandList(&outList);

            //         Dx12MarkerCollector& markerCollector = m_device.markerCollector();
            //         if (markerCollector.isActive())
            //             markerCollector.endMarker(outList);
            //     }
            //     break;
            default:
                CPY_ASSERT_FMT(false, "Unrecognized serialized command %d", cmdType);
                break;
            }
            // applyBarriers(cmdInfo.postBarrier, outList); // TODO
        }
    }

    [cmdBuffer commit];

    return 0; // TODO (Apoorva): Return fence value
}
    
}
}

#endif // ENABLE_METAL