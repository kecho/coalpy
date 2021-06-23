#include <Config.h>
#if ENABLE_DX12
#include "Dx12Resources.h"
#include "Dx12Device.h"
#include "Dx12Formats.h"
#include <coalpy.core/String.h>
#include <string>


namespace coalpy
{
namespace render
{

Dx12Resource::Dx12Resource(Dx12Device& device, const ResourceDesc& config)
: m_device(device)
{
    m_config = config;
    m_data = {};

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
            {
                m_uav = descriptorPool.allocateSrvOrUavOrCbv();
                m_data.resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
        }
        else
        {
            m_data.resDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        m_data.heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        m_usage = Usage::Dynamic; //assume all read / write
        if ((m_config.memFlags & (MemFlag_CpuRead | MemFlag_CpuWrite)) == 0)
        {
            m_usage = Usage::Default;
        }
        else if ((m_config.memFlags & (MemFlag_GpuWrite | MemFlag_GpuRead)) == 0)
        {
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

void Dx12Resource::init()
{
    if (m_ownsResource)
    {
        DX_OK(m_device.device().CreateCommittedResource(
            &m_data.heapProps, m_data.heapFlags, &m_data.resDesc,
	    	defaultState(), nullptr, DX_RET(m_data.resource)));
    }

    if ((m_config.memFlags & MemFlag_GpuRead) != 0)
    {
		m_device.device().CreateShaderResourceView(
			m_data.resource, &m_data.srvDesc, m_srv.handle);
    }

    if ((m_config.memFlags & MemFlag_GpuRead) != 0)
    {
		m_device.device().CreateUnorderedAccessView(
			m_data.resource, nullptr, &m_data.uavDesc, m_uav.handle);
    }

#if ENABLE_RENDER_RESOURCE_NAMES
    {
	    std::wstring wname = wname = s2ws(m_config.name);
	    m_data.resource->SetName(wname.c_str());
    }
#endif
}

void Dx12Resource::acquireD3D12Resource(ID3D12Resource* resource)
{
    m_ownsResource = false;
    m_resolveGpuAddress = false;
    if (m_data.resource != nullptr)
        m_data.resource->Release();

    m_data.resource = resource;

    if (m_data.resource)
        m_data.resource->AddRef();
}

void* Dx12Resource::mappedMemory()
{
    return m_data.mappedMemory;
}

Dx12Resource::~Dx12Resource()
{
    if (m_data.resource)
        m_data.resource->Release();

    auto& descriptorPool = m_device.descriptors();
    if ((m_config.memFlags & MemFlag_GpuRead) != 0)
        descriptorPool.release(m_srv);
    if ((m_config.memFlags & MemFlag_GpuWrite) != 0)
        descriptorPool.release(m_uav);
}

Dx12Texture::Dx12Texture(Dx12Device& device, const TextureDesc& desc)
: Dx12Resource(device, desc)
, m_texDesc(desc)
{
    D3D12_RESOURCE_DIMENSION dim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    if (desc.type == TextureType::k1d)
        dim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    else if (desc.type == TextureType::k2d)
        dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    else if (desc.type == TextureType::k3d)
        dim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    else
        CPY_ASSERT_FMT(false, "Texture type not supported %d yet", desc.type);

    auto dxFormat = getDxFormat(desc.format);

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
            srvDesc.Texture1D.ResourceMinLODClamp = 0u;
        }
        else if (desc.type == TextureType::k2d)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0u;
            srvDesc.Texture2D.MipLevels = desc.mipLevels;
            srvDesc.Texture2D.ResourceMinLODClamp = 0u;
            srvDesc.Texture2D.PlaneSlice = 0u;
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

Dx12Buffer::Dx12Buffer(Dx12Device& device, const BufferDesc& desc)
: Dx12Resource(device, desc)
, m_buffDesc(desc)
{
}

Dx12Buffer::~Dx12Buffer()
{
}

}
}

#endif
