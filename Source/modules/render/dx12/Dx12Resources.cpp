#include <Config.h>
#if ENABLE_DX12
#include "Dx12Resources.h"
#include "Dx12Device.h"
#include "Dx12Formats.h"
#include "Dx12Utils.h"
#include <coalpy.core/String.h>
#include <string>


namespace coalpy
{
namespace render
{

Dx12Resource::Dx12Resource(Dx12Device& device, const ResourceDesc& config, ResourceSpecialFlags specialFlags)
: m_device(device)
{
    m_config = config;
    m_specialFlags = (ResourceSpecialFlags)(specialFlags | (int)(config.recreatable ? ResourceSpecialFlag_TrackTables : ResourceSpecialFlag_None));
    m_data = {};
    const bool canDenyShaderResources = (specialFlags & ResourceSpecialFlag_CanDenyShaderResources) != 0;
    {
        m_data.resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		m_data.heapFlags = D3D12_HEAP_FLAG_NONE;

        //Core resource setup
        if ((m_config.memFlags & MemFlag_GpuWrite) != 0 || (m_config.memFlags & MemFlag_GpuRead) != 0)
        {
            auto& descriptorPool = m_device.descriptors();
            if ((m_config.memFlags & MemFlag_GpuRead) != 0)
            {
                m_srv = descriptorPool.allocateSrvOrUavOrCbv();
            }

            if ((m_config.memFlags & MemFlag_GpuWrite) != 0)
                m_data.resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        else if (canDenyShaderResources)
        {
            m_data.resDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        m_data.heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        m_usage = Usage::Dynamic; //assume all read / write
        if ((specialFlags & (ResourceSpecialFlag_CpuReadback | ResourceSpecialFlag_CpuUpload)) == 0)
        {
            m_usage = Usage::Default;
        }
        else if ((m_config.memFlags & (MemFlag_GpuWrite | MemFlag_GpuRead)) == 0)
        {
            if ((specialFlags & ResourceSpecialFlag_CpuReadback) != 0)
                m_usage = Usage::Readback;
        }

        switch (m_usage)
        {
        case Usage::Default:
            m_defaultState = D3D12_RESOURCE_STATE_COMMON;
            m_data.heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
        case Usage::Dynamic:
		    m_defaultState = D3D12_RESOURCE_STATE_GENERIC_READ;
            m_data.heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            break;
        case Usage::Readback:
		    m_defaultState = D3D12_RESOURCE_STATE_COPY_DEST;
            m_data.heapProps.Type = D3D12_HEAP_TYPE_READBACK;
            break;
        }
    }

    m_data.heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    m_data.heapProps.CreationNodeMask = 0u;
    m_data.heapProps.VisibleNodeMask = 0u;
    m_data.mappedMemory = nullptr;
    m_data.gpuVirtualAddress = {};
}

Dx12ResourceInitResult Dx12Resource::init()
{
    if (m_data.resource == nullptr)
    {
        HRESULT r = m_device.device().CreateCommittedResource(
            &m_data.heapProps, m_data.heapFlags, &m_data.resDesc,
	    	defaultD3d12State(), nullptr, DX_RET(m_data.resource));
        CPY_ASSERT_MSG(r == S_OK, "CreateCommittedResource has failed");
        if (r != S_OK)
            return Dx12ResourceInitResult {  ResourceResult::InvalidParameter, "Failed to call CreateCommittedResource." };
        
        if (isBuffer())
            m_data.gpuVirtualAddress = m_data.resource->GetGPUVirtualAddress();
    }

    if ((m_config.memFlags & MemFlag_GpuRead) != 0)
    {
		m_device.device().CreateShaderResourceView(
			m_data.resource, &m_data.srvDesc, m_srv.handle);
    }
    
    if (m_data.heapProps.Type == D3D12_HEAP_TYPE_READBACK || m_data.heapProps.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
        auto mapResult = m_data.resource->Map(0u, nullptr, &m_data.mappedMemory);
        DX_OK(mapResult);
        if (mapResult != S_OK)
            return Dx12ResourceInitResult {  ResourceResult::Ok, ""  };
    }

#if ENABLE_RENDER_RESOURCE_NAMES
    {
	    std::wstring wname = s2ws(m_config.name);
	    m_data.resource->SetName(wname.c_str());
    }
#endif

    return Dx12ResourceInitResult {  ResourceResult::Ok, ""  };
}

void Dx12Resource::acquireD3D12Resource(ID3D12Resource* resource)
{
    m_resolveGpuAddress = false;
    resource->AddRef();
    if (m_data.resource != nullptr)
    {
        if ((m_specialFlags & ResourceSpecialFlag_NoDeferDelete) == 0)
            m_device.deferRelease(*m_data.resource);
        m_data.resource->Release();
    }

    m_data.resource = resource;
}

void* Dx12Resource::mappedMemory()
{
    return m_data.mappedMemory;
}

Dx12Resource::~Dx12Resource()
{
    if (m_data.resource)
    {
        if (m_data.mappedMemory)
        {
            m_data.resource->Unmap(0u, nullptr);
        }
        m_data.mappedMemory = {};
        
        if ((m_specialFlags & ResourceSpecialFlag_NoDeferDelete) == 0)
            m_device.deferRelease(*m_data.resource);

        m_data.resource->Release();
    }

    auto& descriptorPool = m_device.descriptors();
    if ((m_config.memFlags & MemFlag_GpuRead) != 0)
        descriptorPool.release(m_srv);
    if ((m_config.memFlags & MemFlag_GpuWrite) != 0)
    {
        for (auto& uav : m_uavs)
            descriptorPool.release(uav);
        m_uavs.clear();
    }
}

Dx12Texture::Dx12Texture(Dx12Device& device, const TextureDesc& desc, ResourceSpecialFlags specialFlags)
: Dx12Resource(device, desc, specialFlags)
, m_texDesc(desc)
{
    if (desc.isRtv)
        m_data.resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_RESOURCE_DIMENSION dim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    if (desc.type == TextureType::k1d)
        dim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    else if (desc.type == TextureType::k2d || desc.type == TextureType::k2dArray || desc.type == TextureType::CubeMap || desc.type == TextureType::CubeMapArray)
        dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    else if (desc.type == TextureType::k3d)
        dim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    else
        CPY_ASSERT_FMT(false, "Texture type not supported %d yet", desc.type);

    auto dxFormat = getDxFormat(desc.format);

    m_slices = desc.depth;
    if (desc.type == TextureType::CubeMapArray)
    {
        m_slices *= 6;
    }
    else if (desc.type == TextureType::CubeMap)
    {
        m_slices = 6;
    }
    else if (desc.type != TextureType::k2dArray)
        m_slices = 1;


    if ((desc.memFlags & MemFlag_GpuRead) != 0)
    {
        auto& srvDesc = m_data.srvDesc;
        srvDesc.Format = dxFormat;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (desc.type == TextureType::k1d)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MostDetailedMip = 0u;
            srvDesc.Texture1D.MipLevels = desc.mipLevels;
            srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
        }
        else if (desc.type == TextureType::k2d)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0u;
            srvDesc.Texture2D.MipLevels = desc.mipLevels;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2D.PlaneSlice = 0u;
        }
        else if (desc.type == TextureType::k2dArray)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0u;
            srvDesc.Texture2DArray.MipLevels = desc.mipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = 0u;
            srvDesc.Texture2DArray.ArraySize = desc.depth;
            srvDesc.Texture2DArray.PlaneSlice = 0u;
            srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        }
        else if (desc.type == TextureType::CubeMap)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MostDetailedMip = 0;
            srvDesc.TextureCube.MipLevels = desc.mipLevels;
            srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        }
        else if (desc.type == TextureType::CubeMapArray)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            srvDesc.TextureCubeArray.MostDetailedMip = 0u;
            srvDesc.TextureCubeArray.MipLevels = desc.mipLevels;
            srvDesc.TextureCubeArray.First2DArrayFace = 0;
            srvDesc.TextureCubeArray.NumCubes = desc.depth;
            srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
        }
        else if (desc.type == TextureType::k3d)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = 0u;
            srvDesc.Texture3D.MipLevels = desc.mipLevels;
            srvDesc.Texture3D.ResourceMinLODClamp = 0u;
        }
    }

    if ((desc.memFlags & MemFlag_GpuWrite) != 0)
    {
        auto& uavDesc = m_data.uavDesc;
        uavDesc.Format = dxFormat;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_UNKNOWN;
        if (desc.type == TextureType::k1d)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = 0u;
        }
        else if (desc.type == TextureType::k2d)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0u;
            uavDesc.Texture2D.PlaneSlice = 0u;
        }
        else if (desc.type == TextureType::k3d)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.MipSlice = 0u;
            uavDesc.Texture3D.FirstWSlice = 0u;
            uavDesc.Texture3D.WSize = -1;
        }
        else if (desc.type == TextureType::k2dArray || desc.type == TextureType::CubeMapArray || desc.type == TextureType::CubeMap)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.MipSlice = 0;
            uavDesc.Texture2DArray.FirstArraySlice = 0;
            uavDesc.Texture2DArray.ArraySize = (UINT)m_slices;
            uavDesc.Texture2DArray.PlaneSlice = 0;
        }
    }

    m_data.heapFlags |= D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    m_data.resDesc.Format = dxFormat;
    m_data.resDesc.Dimension = dim;
    m_data.resDesc.Alignment = 0u;
    m_data.resDesc.Width = desc.width;
    m_data.resDesc.Height = desc.height;
    
    m_data.resDesc.DepthOrArraySize = desc.depth;
    m_data.resDesc.MipLevels = desc.mipLevels;
    m_data.resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    m_data.resDesc.SampleDesc.Count = 1u;
    m_data.resDesc.SampleDesc.Quality = 0u;
}

Dx12Texture::~Dx12Texture()
{
}

size_t Dx12Texture::byteSize() const
{
    int width, height, depth;
    size_t rowPitch;
    getCpuTextureSizes(0u, rowPitch, width, height, depth);
    return rowPitch * height * depth;
}

void Dx12Texture::getCpuTextureSizes(int mipId, size_t& outRowPitch, int& outWidth, int& outHeight, int& outDepth) const
{
    CPY_ASSERT(mipId >=0 && mipId < m_data.resDesc.MipLevels);

    outWidth = (int)m_data.resDesc.Width >> mipId;
    outHeight = (int)m_data.resDesc.Height >> mipId;
    outDepth = (int)m_data.resDesc.DepthOrArraySize;

    if (m_data.resDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
        outDepth = outDepth >> mipId;

    outWidth = outWidth >= 1 ? outWidth : 1;
    outHeight = outHeight >= 1 ? outHeight : 1;
    outDepth = outDepth >= 1 ? outDepth : 1;
    outRowPitch = (size_t)alignByte(getDxFormatStride(m_texDesc.format) * outWidth, (int)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
}

Dx12ResourceInitResult Dx12Texture::init()
{
    auto parentResult = Dx12Resource::init();
    if (!parentResult.success())
        return parentResult;
    
    if ((m_config.memFlags & MemFlag_GpuWrite) != 0)
    {
        auto& descriptorPool = m_device.descriptors();
        for (int mipId = 0; mipId < m_texDesc.mipLevels; ++mipId)
        {
            auto uav = descriptorPool.allocateSrvOrUavOrCbv();
            auto mipUavDesc = m_data.uavDesc;
            if (mipUavDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
                mipUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

            switch (m_data.uavDesc.ViewDimension)
            {
            case D3D12_UAV_DIMENSION_TEXTURE1D:
                mipUavDesc.Texture1D.MipSlice = mipId;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2D:
                mipUavDesc.Texture2D.MipSlice = mipId;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                mipUavDesc.Texture2DArray.MipSlice = mipId;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                mipUavDesc.Texture3D.MipSlice = mipId;
                break;
            }
            m_device.device().CreateUnorderedAccessView(
            	m_data.resource, nullptr, &mipUavDesc, uav.handle);
            m_uavs.push_back(uav);
        }
    }

    return Dx12ResourceInitResult { ResourceResult::Ok };
}

Dx12Buffer::Dx12Buffer(Dx12Device& device, const BufferDesc& desc, ResourceSpecialFlags specialFlags)
: Dx12Resource(device, desc, ResourceSpecialFlags(specialFlags | ResourceSpecialFlag_CanDenyShaderResources))
, m_buffDesc(desc)
{
    if ((desc.memFlags & MemFlag_GpuRead) != 0)
    {
        auto& srvDesc = m_data.srvDesc;
        srvDesc.Format = desc.type == BufferType::Standard ? getDxFormat(desc.format) :  ( desc.type == BufferType::Raw ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN);
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0u;
        srvDesc.Buffer.NumElements = desc.elementCount;
        srvDesc.Buffer.StructureByteStride = desc.type == BufferType::Structured ? desc.stride : 0;
        srvDesc.Buffer.Flags = desc.type == BufferType::Raw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
    }

    if ((desc.memFlags & MemFlag_GpuWrite) != 0)
    {
        auto& uavDesc = m_data.uavDesc;
        uavDesc.Format = desc.type == BufferType::Standard ? getDxFormat(desc.format) :  ( desc.type == BufferType::Raw ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN);
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0u;
        uavDesc.Buffer.NumElements = desc.elementCount;
        uavDesc.Buffer.StructureByteStride = desc.type == BufferType::Structured ? desc.stride : 0;
        uavDesc.Buffer.CounterOffsetInBytes = 0u;
        uavDesc.Buffer.Flags = desc.type == BufferType::Raw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
    }

    m_data.resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    m_data.resDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    m_data.resDesc.Width = (desc.type == BufferType::Standard
                           ? getDxFormatStride(desc.format)
                           : desc.stride) * desc.elementCount;

    if (m_buffDesc.isConstantBuffer)
        m_data.resDesc.Width = alignByte<UINT>(m_data.resDesc.Width, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    m_data.resDesc.Height = 1u;
    m_data.resDesc.DepthOrArraySize = 1u;
    m_data.resDesc.MipLevels = 1u;
    m_data.resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //m_data.resDesc.Flags = check Dx12Resource constructor;
    m_data.resDesc.Format = DXGI_FORMAT_UNKNOWN;
    m_data.resDesc.SampleDesc.Count = 1u;
    m_data.resDesc.SampleDesc.Quality = 0u;
}

Dx12ResourceInitResult Dx12Buffer::init()
{
    auto parentResult = Dx12Resource::init();
    if (!parentResult.success())
        return parentResult;

    if ((m_config.memFlags & MemFlag_GpuWrite) != 0)
    {
        auto& descriptorPool = m_device.descriptors();
        if (m_data.uavDesc.ViewDimension == D3D12_UAV_DIMENSION_BUFFER)
        {
            m_uavs.push_back(descriptorPool.allocateSrvOrUavOrCbv());
		    m_device.device().CreateUnorderedAccessView(
		    	m_data.resource, nullptr, &m_data.uavDesc, m_uavs.back().handle);
        }
    }

    if (m_buffDesc.isConstantBuffer)
    {
        m_cbv = m_device.descriptors().allocateSrvOrUavOrCbv();

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = m_data.gpuVirtualAddress;
        cbvDesc.SizeInBytes = m_data.resDesc.Width;
        m_device.device().CreateConstantBufferView(
            &cbvDesc, m_cbv.handle);
    }

    return Dx12ResourceInitResult { ResourceResult::Ok };
}

Dx12Buffer::~Dx12Buffer()
{
    if (m_buffDesc.isConstantBuffer)
        m_device.descriptors().release(m_cbv);
}

Dx12ResourceTable::Dx12ResourceTable(Dx12Device& device, Dx12Resource** resources, int count, bool isUav)
: m_device(device)
{
    std::vector<Dx12Descriptor> descriptors;
    m_resources.insert(m_resources.end(), resources, resources + count);
    for (auto& r : m_resources)
    {
        if (!r)
            continue;
        descriptors.push_back(isUav ? r->uav() : r->srv());
    }

    m_cpuTable = m_device.descriptors().allocateTable(
        Dx12DescriptorTableType::SrvCbvUav,
        descriptors.data(), (int)descriptors.size());
        
}

Dx12ResourceTable::~Dx12ResourceTable()
{
    m_device.descriptors().release(m_cpuTable);
}

size_t Dx12Buffer::byteSize() const
{
    return m_data.resDesc.Width;
}

}
}

#endif
