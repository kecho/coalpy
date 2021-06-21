#include <Config.h>
#if ENABLE_DX12
#include "Dx12Resources.h"
#include "Dx12Device.h"
#include <coalpy.core/String.h>
#include <string>


namespace coalpy
{
namespace render
{

Dx12Resource::Dx12Resource(Dx12Device& device, const Dx12ResourceConfig& config)
: m_device(device), m_config(config)
{
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
    }

    m_data.heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
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

Dx12Resource::~Dx12Resource()
{
    if (m_data.resource)
        m_data.resource->Release();
}

}
}

#endif
