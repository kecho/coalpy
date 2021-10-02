#include "Dx12WorkBundle.h"
#include "Dx12Device.h"
#include "Dx12Resources.h"
#include "Dx12ResourceCollection.h"
#include "Dx12ShaderDb.h"
#include "Dx12Utils.h"
#include "Dx12Formats.h"
#include "Dx12CounterPool.h"

namespace coalpy
{
namespace render
{

bool Dx12WorkBundle::load(const WorkBundle& workBundle)
{
    m_workBundle = workBundle;
    for (auto& it : workBundle.states)
        m_states[it.first] = getDx12GpuState(it.second.state);

    return true;
}

void Dx12WorkBundle::uploadAllTables()
{
    int totalDescriptors = m_workBundle.totalTableSize + m_workBundle.totalConstantBuffers;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescBase;
    srcDescBase.reserve(totalDescriptors);

    std::vector<UINT> srcDescCounts;
    srcDescCounts.reserve(totalDescriptors);

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dstDescBase;
    dstDescBase.reserve(totalDescriptors);

    std::vector<UINT> dstDescCounts;
    dstDescCounts.reserve(totalDescriptors);

    int totalSamplers = m_workBundle.totalSamplers;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcSamplers;
    srcSamplers.reserve(totalSamplers);

    std::vector<UINT> srcSamplersCounts;
    srcSamplersCounts.reserve(totalSamplers);

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dstSamplers;
    dstSamplers.reserve(totalSamplers);

    std::vector<UINT> dstSamplersCounts;
    dstSamplersCounts.reserve(totalSamplers);

    Dx12ResourceCollection& resources = m_device.resources();
    if (m_srvUavTable.ownerHeap != nullptr && m_workBundle.totalTableSize)
    {
        for (auto& it : m_workBundle.tableAllocations)
        {
            CPY_ASSERT(it.first.valid());
            Dx12ResourceTable& t = resources.unsafeGetTable(it.first);
            const Dx12DescriptorTable& cpuTable = t.cpuTable();
                
            if (it.second.isSampler)
            {
                srcSamplers.push_back(cpuTable.baseHandle);
                srcSamplersCounts.push_back(cpuTable.count);
        
                dstSamplers.push_back(m_samplersTable.getCpuHandle(it.second.offset));
                dstSamplersCounts.push_back(it.second.count);
            }
            else
            {
                srcDescBase.push_back(cpuTable.baseHandle);
                srcDescCounts.push_back(cpuTable.count);

                dstDescBase.push_back(m_srvUavTable.getCpuHandle(it.second.offset));
                dstDescCounts.push_back(it.second.count);
            }
            CPY_ASSERT(it.second.count == cpuTable.count);
        }
    }
    if (!dstDescBase.empty())
    {
        m_device.device().CopyDescriptors(
            (UINT)dstDescBase.size(),
            dstDescBase.data(), dstDescCounts.data(),
            (UINT)srcDescBase.size(),
            srcDescBase.data(), srcDescCounts.data(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    if (!dstSamplers.empty())
    {
        m_device.device().CopyDescriptors(
            (UINT)dstSamplers.size(),
            dstSamplers.data(), dstSamplersCounts.data(),
            (UINT)srcSamplers.size(),
            srcSamplers.data(), srcSamplersCounts.data(),
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }
}

void Dx12WorkBundle::applyBarriers(const std::vector<ResourceBarrier>& barriers, ID3D12GraphicsCommandListX& outList)
{
    if (barriers.empty())
        return;

    std::vector<D3D12_RESOURCE_BARRIER> resultBarriers;
    resultBarriers.reserve(barriers.size());
    Dx12ResourceCollection& resources = m_device.resources();

    for (const auto& b : barriers)
    {
        if (!b.isUav && b.prevState == b.postState)
            continue;

        Dx12Resource& r = resources.unsafeGetResource(b.resource);
        
        resultBarriers.emplace_back();
        D3D12_RESOURCE_BARRIER& d3d12barrier = resultBarriers.back();
        d3d12barrier = {};
        if (b.type == BarrierType::Begin)
        {
            d3d12barrier.Flags |= D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
        }
        else if (b.type == BarrierType::End)
        {
            d3d12barrier.Flags |= D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
        }

        if (b.isUav)
        {
            d3d12barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            d3d12barrier.UAV.pResource = &r.d3dResource();
        }
        else
        {
            d3d12barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            d3d12barrier.Transition.pResource = &r.d3dResource();
            d3d12barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            d3d12barrier.Transition.StateBefore = getDx12GpuState(b.prevState);
            d3d12barrier.Transition.StateAfter  = getDx12GpuState(b.postState);
        }
    }

    if (resultBarriers.empty())
        return;

    outList.ResourceBarrier((UINT)resultBarriers.size(), resultBarriers.data());
}

void Dx12WorkBundle::buildComputeCmd(const unsigned char* data, const AbiComputeCmd* computeCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList)
{
    Dx12ShaderDb& db = m_device.shaderDb();

    //TODO: optimize this resolve.
    db.resolve(computeCmd->shader);
    ID3D12PipelineState* pso = db.unsafeGetCsPso(computeCmd->shader);
    CPY_ASSERT(pso != nullptr);
    if (pso == nullptr)
        return;

    //Activate this if there are synchronization issues.
    #if 0
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    outList.ResourceBarrier(1, &uavBarrier);
    #endif

    //TODO: set this once per cmd list
    outList.SetComputeRootSignature(&m_device.defaultComputeRootSignature());

    //TODO: prune this.
    ID3D12DescriptorHeap* heaps[2] = {};
    int listCounts = 0;
    if (m_srvUavTable.ownerHeap)
        heaps[listCounts++] = m_srvUavTable.ownerHeap;
    if (m_samplersTable.ownerHeap)
        heaps[listCounts++] = m_samplersTable.ownerHeap;
    outList.SetDescriptorHeaps(listCounts, heaps);

    outList.SetPipelineState(pso);

    //TODO: this can be taken outside
    if (computeCmd->inlineConstantBufferSize > 0)
    {
        CPY_ASSERT(computeCmd->inlineConstantBufferSize <= (m_uploadMemBlock.uploadSize - cmdInfo.uploadBufferOffset));
        memcpy((char*)m_uploadMemBlock.mappedBuffer + cmdInfo.uploadBufferOffset, computeCmd->inlineConstantBuffer.data(data), computeCmd->inlineConstantBufferSize);
        
        //HACK: dx12 requires aligned buffers to be 256, this size has been hacked to be aligned in WorkBundleDb, processCompte()
        UINT alignedSize = alignByte(computeCmd->inlineConstantBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbufferDesc = { m_uploadMemBlock.gpuVA + cmdInfo.uploadBufferOffset, alignedSize };
        m_device.device().CreateConstantBufferView(&cbufferDesc, m_cbvTable.getCpuHandle(cmdInfo.constantBufferTableOffset));
        outList.SetComputeRootDescriptorTable(m_device.tableIndex(Dx12Device::TableTypes::Cbv, 0), m_cbvTable.getGpuHandle(cmdInfo.constantBufferTableOffset)); //one and only one table for cbv
    }
    else if (computeCmd->constantCounts > 0)
    {
        CPY_ASSERT(cmdInfo.constantBufferTableOffset >= 0);
        const Buffer* buffers = computeCmd->constants.data(data);
        for (int cbufferId = 0; cbufferId < computeCmd->constantCounts; ++cbufferId)
        {
            Dx12Resource& resource = m_device.resources().unsafeGetResource(buffers[cbufferId]);
            CPY_ASSERT(resource.isBuffer());
            Dx12Buffer& buff = (Dx12Buffer&)resource;
            m_device.device().CopyDescriptorsSimple(1, m_cbvTable.getCpuHandle(cbufferId + cmdInfo.constantBufferTableOffset), buff.cbv().handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        D3D12_GPU_DESCRIPTOR_HANDLE descriptor = m_cbvTable.getGpuHandle(cmdInfo.constantBufferTableOffset);
        outList.SetComputeRootDescriptorTable(m_device.tableIndex(Dx12Device::TableTypes::Cbv, 0), descriptor); //one and oonly one table for cbv
    }

    const InResourceTable* inTables = computeCmd->inResourceTables.data(data);
    for (int inTableId = 0; inTableId < computeCmd->inResourceTablesCounts; ++inTableId)
    {
        auto it = m_workBundle.tableAllocations.find((ResourceTable)inTables[inTableId]);
        CPY_ASSERT(it != m_workBundle.tableAllocations.end());
        if (it == m_workBundle.tableAllocations.end())
            return;

        D3D12_GPU_DESCRIPTOR_HANDLE descriptor = m_srvUavTable.getGpuHandle(it->second.offset);
        outList.SetComputeRootDescriptorTable(m_device.tableIndex(Dx12Device::TableTypes::Srv, inTableId) , descriptor);
    }

    const OutResourceTable* outTables = computeCmd->outResourceTables.data(data);
    for (int outTableId = 0; outTableId < computeCmd->outResourceTablesCounts; ++outTableId)
    {
        auto it = m_workBundle.tableAllocations.find((ResourceTable)outTables[outTableId]);
        CPY_ASSERT(it != m_workBundle.tableAllocations.end());
        if (it == m_workBundle.tableAllocations.end())
            return;

        D3D12_GPU_DESCRIPTOR_HANDLE descriptor = m_srvUavTable.getGpuHandle(it->second.offset);
        outList.SetComputeRootDescriptorTable(m_device.tableIndex(Dx12Device::TableTypes::Uav, outTableId) , descriptor);
    }

    const SamplerTable* samplerTables = computeCmd->samplerTables.data(data);
    for (int samplerTableId = 0; samplerTableId < computeCmd->samplerTablesCounts; ++samplerTableId)
    {
        auto it = m_workBundle.tableAllocations.find((ResourceTable)samplerTables[samplerTableId]);
        CPY_ASSERT(it != m_workBundle.tableAllocations.end());
        if (it == m_workBundle.tableAllocations.end())
            return;
    
        D3D12_GPU_DESCRIPTOR_HANDLE descriptor = m_samplersTable.getGpuHandle(it->second.offset);
        outList.SetComputeRootDescriptorTable(m_device.tableIndex(Dx12Device::TableTypes::Sampler, samplerTableId), descriptor);
    }

    if (computeCmd->isIndirect)
    {
        Dx12Resource& indirectBuffer = m_device.resources().unsafeGetResource(computeCmd->indirectArguments);
        outList.ExecuteIndirect(&m_device.indirectDispatchSignature(), 1u, &indirectBuffer.d3dResource(), 0u, nullptr, 0u);
    }
    else
    {
        outList.Dispatch(computeCmd->x, computeCmd->y, computeCmd->z);
    }
}

void Dx12WorkBundle::buildCopyCmd(const unsigned char* data, const AbiCopyCmd* copyCmd, ID3D12GraphicsCommandListX& outList)
{
    Dx12ResourceCollection& resources = m_device.resources();
    Dx12Resource& src = resources.unsafeGetResource(copyCmd->source);
    Dx12Resource& dst = resources.unsafeGetResource(copyCmd->destination);
    if (copyCmd->fullCopy)
        outList.CopyResource(&dst.d3dResource(), &src.d3dResource());
    else if (src.isBuffer())
    {
        UINT64 sizeToCopy;
        if (copyCmd->sizeX >= 0)
        {
            sizeToCopy = copyCmd->sizeX;
        }
        else
        {
            int dstRemainingSize = ((int)dst.byteSize() - copyCmd->destX);
            int srcRemainingSize = ((int)src.byteSize() - copyCmd->sourceX);
            sizeToCopy = min(dstRemainingSize, srcRemainingSize);
        }

        outList.CopyBufferRegion(
            &dst.d3dResource(), (UINT64)copyCmd->destX,
            &src.d3dResource(), (UINT64)copyCmd->sourceX, sizeToCopy);
    }
    else
    {
        Dx12Texture& srcTexture = (Dx12Texture&)src;
        Dx12Texture& dstTexture = (Dx12Texture&)dst;

        const auto& srcDesc = srcTexture.texDesc();
        const auto& dstDesc = dstTexture.texDesc();

        auto zAsSlice = [](TextureType t) { return t == TextureType::k2dArray || t == TextureType::CubeMapArray || t == TextureType::CubeMap; };
        D3D12_TEXTURE_COPY_LOCATION srcLocation;
        srcLocation.pResource = &srcTexture.d3dResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        bool srcZAsSlice = zAsSlice(dstDesc.type);
        int srcSliceId = srcZAsSlice ? copyCmd->sourceZ : 0;
        srcLocation.SubresourceIndex = (UINT)srcTexture.subresourceIndex(copyCmd->srcMipLevel, srcSliceId);

        D3D12_TEXTURE_COPY_LOCATION dstLocation;
        dstLocation.pResource = &dstTexture.d3dResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        bool dstZAsSlice = zAsSlice(dstDesc.type);
        int dstSliceId = dstZAsSlice ? copyCmd->destZ : 0;
        dstLocation.SubresourceIndex = (UINT)dstTexture.subresourceIndex(copyCmd->dstMipLevel, dstSliceId);

        int szX = copyCmd->sizeX < 0 ? (min(srcDesc.width  - copyCmd->sourceX, dstDesc.width   - copyCmd->destX)) : copyCmd->sizeX;
        int szY = copyCmd->sizeY < 0 ? (min(srcDesc.height - copyCmd->sourceY, dstDesc.height  - copyCmd->destY)) : copyCmd->sizeY;
        int szZ = copyCmd->sizeZ < 0 ? (min(srcDesc.depth  - copyCmd->sourceZ, dstDesc.depth   - copyCmd->destZ)) : copyCmd->sizeZ;

        D3D12_BOX box;
        box.left  = (UINT)copyCmd->sourceX;
        box.top   = (UINT)copyCmd->sourceY;
        box.front = srcZAsSlice ? 0 : (UINT)copyCmd->sourceZ;
        box.right  = box.left  + (UINT)szX; 
        box.bottom = box.top   + (UINT)szY; 
        box.back   = box.front + (srcZAsSlice ? 1 : (UINT)szZ);

        outList.CopyTextureRegion(
            &dstLocation, (UINT)copyCmd->destX, (UINT)copyCmd->destY, dstZAsSlice ? 0 : (UINT)copyCmd->destZ,
            &srcLocation, &box);
    }
}

void Dx12WorkBundle::buildDownloadCmd(
    const unsigned char* data, const AbiDownloadCmd* downloadCmd,
    const CommandInfo& cmdInfo, WorkType workType,
    ID3D12GraphicsCommandListX& outList)
{
    Dx12ResourceCollection& resources = m_device.resources();
    CPY_ASSERT(cmdInfo.commandDownloadIndex >= 0 && cmdInfo.commandDownloadIndex < (int)m_downloadStates.size());
    CPY_ASSERT(downloadCmd->source.valid());
    auto& downloadState = m_downloadStates[cmdInfo.commandDownloadIndex];
    downloadState.downloadKey = ResourceDownloadKey { downloadCmd->source, downloadCmd->mipLevel, downloadCmd->arraySlice };
    Dx12Resource& dx12Resource = resources.unsafeGetResource(downloadCmd->source);
    if (dx12Resource.isBuffer())
    {
        downloadState.memoryBlock = m_device.readbackPool().allocate(dx12Resource.byteSize());
        outList.CopyBufferRegion(downloadState.memoryBlock.buffer, downloadState.memoryBlock.offset, &dx12Resource.d3dResource(), 0u, dx12Resource.byteSize());
    }
    else
    {
        auto& dx12Texture = (Dx12Texture&)dx12Resource;
        int unused = 0;
        dx12Texture.getCpuTextureSizes(downloadCmd->mipLevel, unused, downloadState.rowPitch, downloadState.width, downloadState.height, downloadState.depth);
        downloadState.memoryBlock = m_device.readbackPool().allocate(downloadState.rowPitch * downloadState.height * downloadState.depth);

        D3D12_TEXTURE_COPY_LOCATION srcLoc;
        srcLoc.pResource = &dx12Texture.d3dResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = (UINT)dx12Texture.subresourceIndex(downloadCmd->mipLevel, downloadCmd->arraySlice);

        D3D12_TEXTURE_COPY_LOCATION dstLoc;
        dstLoc.pResource = downloadState.memoryBlock.buffer;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLoc.PlacedFootprint.Offset = (UINT)downloadState.memoryBlock.offset;
        dstLoc.PlacedFootprint.Footprint.Format = dx12Resource.d3dResDesc().Format;
        dstLoc.PlacedFootprint.Footprint.Width = (UINT)downloadState.width;
        dstLoc.PlacedFootprint.Footprint.Height = (UINT)downloadState.height;
        dstLoc.PlacedFootprint.Footprint.Depth = (UINT)downloadState.depth;
        dstLoc.PlacedFootprint.Footprint.RowPitch = (UINT)downloadState.rowPitch;

        outList.CopyTextureRegion(
            &dstLoc, 0u, 0u, 0u,
            &srcLoc, nullptr);
    }
    downloadState.queueType = workType;
    downloadState.fenceValue = m_currentFenceValue + 1;
    downloadState.resource = downloadCmd->source;
}

void Dx12WorkBundle::buildUploadCmd(const unsigned char* data, const AbiUploadCmd* uploadCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList)
{
    Dx12ResourceCollection& resources = m_device.resources();
    Dx12Resource& destinationResource = resources.unsafeGetResource(uploadCmd->destination);
    CPY_ASSERT(cmdInfo.uploadBufferOffset < m_uploadMemBlock.uploadSize);
    CPY_ASSERT((m_uploadMemBlock.uploadSize - cmdInfo.uploadBufferOffset) >= uploadCmd->sourceSize);
    if (destinationResource.isBuffer())
    {
        //TODO: this can be jobified.
        {
            memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + cmdInfo.uploadBufferOffset, uploadCmd->sources.data(data), uploadCmd->sourceSize);
        }

        outList.CopyBufferRegion(
            &destinationResource.d3dResource(),
            (UINT)uploadCmd->destX,
            m_uploadMemBlock.buffer,
            m_uploadMemBlock.offset + cmdInfo.uploadBufferOffset,
            uploadCmd->sourceSize);
    }
    else
    {
        Format format = ((Dx12Texture&)destinationResource).texDesc().format;
        int formatStride = getDxFormatStride(format);

        CPY_ASSERT(formatStride == cmdInfo.uploadDestinationMemoryInfo.texelElementPitch);

        auto zAsSlice = [](TextureType t) { return t == TextureType::k2dArray || t == TextureType::CubeMapArray || t == TextureType::CubeMap; };
        Dx12Texture& destTexture = (Dx12Texture&)destinationResource;
        bool useZAsSlice = zAsSlice(destTexture.texDesc().type);


        D3D12_TEXTURE_COPY_LOCATION destTexLoc;
        destTexLoc.pResource = &destinationResource.d3dResource();
        destTexLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; 
        destTexLoc.SubresourceIndex = destTexture.subresourceIndex(uploadCmd->mipLevel,  useZAsSlice ? uploadCmd->destZ : 0);

        int szX = uploadCmd->sizeX < 0 ? (cmdInfo.uploadDestinationMemoryInfo.width  - uploadCmd->destX) : uploadCmd->sizeX;
        int szY = uploadCmd->sizeY < 0 ? (cmdInfo.uploadDestinationMemoryInfo.height - uploadCmd->destY) : uploadCmd->sizeY;
        int szZ = uploadCmd->sizeZ < 0 ? (cmdInfo.uploadDestinationMemoryInfo.depth  - uploadCmd->destZ) : uploadCmd->sizeZ;

        szZ = useZAsSlice ? 1 : szZ;

        int segments = szY * szZ;
        int sourceRowPitch = szX * formatStride;
        if ((sourceRowPitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) == 0) //is aligned!
        {
            memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + cmdInfo.uploadBufferOffset, uploadCmd->sources.data(data), sourceRowPitch * segments);
        }
        else
        {
            int dstOffset = cmdInfo.uploadBufferOffset;
            int srcOffset = 0;
            for (int s = 0; s < segments; ++s)
            {
                memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + dstOffset, uploadCmd->sources.data(data) + srcOffset, sourceRowPitch);
                dstOffset += cmdInfo.uploadDestinationMemoryInfo.rowPitch;
                srcOffset += sourceRowPitch;
            }
        }

        auto& destDesc = destinationResource.d3dResDesc();
        D3D12_TEXTURE_COPY_LOCATION srcTexLoc;
        srcTexLoc.pResource = m_uploadMemBlock.buffer;
        srcTexLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcTexLoc.PlacedFootprint.Offset = m_uploadMemBlock.offset + cmdInfo.uploadBufferOffset;
        srcTexLoc.PlacedFootprint.Footprint.Format = destDesc.Format;
        srcTexLoc.PlacedFootprint.Footprint.Width = szX;
        srcTexLoc.PlacedFootprint.Footprint.Height = szY;
        srcTexLoc.PlacedFootprint.Footprint.Depth = szZ;
        srcTexLoc.PlacedFootprint.Footprint.RowPitch = (UINT)cmdInfo.uploadDestinationMemoryInfo.rowPitch;

        outList.CopyTextureRegion(
            &destTexLoc,
            uploadCmd->destX, uploadCmd->destY, uploadCmd->destZ,
            &srcTexLoc,
            nullptr);
    }

}

void Dx12WorkBundle::buildClearAppendConsumeCounter(const unsigned char* data, const AbiClearAppendConsumeCounter* abiCmd, const CommandInfo& cmdInfo, ID3D12GraphicsCommandListX& outList)
{
    Dx12ResourceCollection& resources = m_device.resources();
    Dx12Resource& destinationResource = resources.unsafeGetResource(abiCmd->source);
    CPY_ASSERT(cmdInfo.uploadBufferOffset < m_uploadMemBlock.uploadSize);
    CPY_ASSERT(4u <= (m_uploadMemBlock.uploadSize - cmdInfo.uploadBufferOffset));
    if (destinationResource.isBuffer())
    {
        //TODO: this can be jobified.
        {
            static unsigned int intZeroValue = 0u;
            memcpy(((unsigned char*)m_uploadMemBlock.mappedBuffer) + cmdInfo.uploadBufferOffset, &intZeroValue, 4u);
        }

        outList.CopyBufferRegion(
            &m_device.counterPool().resource(),
            m_device.counterPool().counterOffset(destinationResource.counterHandle()),
            m_uploadMemBlock.buffer,
            m_uploadMemBlock.offset + cmdInfo.uploadBufferOffset,
            4u);
    }
}

void Dx12WorkBundle::buildCommandList(int listIndex, const CommandList* cmdList, WorkType workType, ID3D12GraphicsCommandListX& outList)
{
    CPY_ASSERT(cmdList->isFinalized());
    const unsigned char* listData = cmdList->data();
    const ProcessedList& pl = m_workBundle.processedLists[listIndex];
    for (int commandIndex = 0; commandIndex < pl.commandSchedule.size(); ++commandIndex)
    {
        const CommandInfo& cmdInfo = pl.commandSchedule[commandIndex];
        const unsigned char* cmdBlob = listData + cmdInfo.commandOffset;
        AbiCmdTypes cmdType = *((AbiCmdTypes*)cmdBlob);
        applyBarriers(cmdInfo.preBarrier, outList);
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
                buildCopyCmd(listData, abiCmd, outList);
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
                buildDownloadCmd(listData, abiCmd, cmdInfo, workType, outList);
            }
            break;
        case AbiCmdTypes::ClearAppendConsumeCounter:
            {
                const auto* abiCmd = (const AbiClearAppendConsumeCounter*)cmdBlob;
                buildClearAppendConsumeCounter(listData, abiCmd, cmdInfo, outList);
            }
            break;
        default:
            CPY_ASSERT_FMT(false, "Unrecognized serialized command %d", cmdType);
            return;
        }
        applyBarriers(cmdInfo.postBarrier, outList);
    }

    outList.Close();
}

UINT64 Dx12WorkBundle::execute(CommandList** commandLists, int commandListsCount)
{
    CPY_ASSERT(commandListsCount == (int)m_workBundle.processedLists.size());

    WorkType workType = WorkType::Graphics;
    auto& queues = m_device.queues();
    ID3D12CommandQueue& cmdQueue = queues.cmdQueue(workType);
    Dx12MemoryPools& pools = queues.memPools(workType);

    pools.uploadPool->beginUsage();
    pools.tablePool->beginUsage();
    pools.samplerPool->beginUsage();
    m_downloadStates.resize((int)m_workBundle.resourcesToDownload.size());

    if (m_workBundle.totalUploadBufferSize)
        m_uploadMemBlock = pools.uploadPool->allocUploadBlock(m_workBundle.totalUploadBufferSize);

    if ((m_workBundle.totalTableSize + m_workBundle.totalConstantBuffers) > 0)
    {
        Dx12GpuDescriptorTable table = pools.tablePool->allocateTable(m_workBundle.totalTableSize + m_workBundle.totalConstantBuffers);
        m_srvUavTable = table;
        table.advance(m_workBundle.totalTableSize);
        m_cbvTable = table;
    }

    if (m_workBundle.totalSamplers > 0)
        m_samplersTable = pools.samplerPool->allocateTable(m_workBundle.totalSamplers);

    uploadAllTables();

    std::vector<Dx12List> lists;
    std::vector<ID3D12CommandList*> dx12Lists;
    m_currentFenceValue = queues.currentFenceValue(workType);
    for (int i = 0; i < commandListsCount; ++i)
    {
        lists.emplace_back();
        Dx12List& list = lists.back();
        queues.allocate(workType, list);
        dx12Lists.push_back(list.list);

        buildCommandList(i, commandLists[i], workType, *list.list);
    }

    cmdQueue.ExecuteCommandLists(dx12Lists.size(), dx12Lists.data());

    UINT64 fenceVal = queues.signalFence(workType);
    for (auto l : lists)
        queues.deallocate(l, fenceVal);

    pools.samplerPool->endUsage();
    pools.tablePool->endUsage();
    pools.uploadPool->endUsage();
    return fenceVal;
}

void Dx12WorkBundle::getDownloadResourceMap(Dx12DownloadResourceMap& downloadMap)
{
    for (auto& state : m_downloadStates)
        downloadMap[state.downloadKey] = state;
}

}
}
