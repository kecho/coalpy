#pragma once

#include <D3D12.h>
#include <coalpy.render/Resources.h>
#include <string>

namespace coalpy
{
namespace render
{

class Dx12Device;

struct Dx12ResourceConfig
{
    std::string name;
    MemFlags memFlags;
};

class Dx12Resource
{
public:
    Dx12Resource(Dx12Device& device, const Dx12ResourceConfig& config);
    virtual ~Dx12Resource();

    D3D12_RESOURCE_STATES defaultState() const { return m_defaultState; }
    void acquireD3D12Resource(ID3D12Resource* resource);
    ID3D12Resource& d3dResource() const { return *m_data.resource; }
    ID3D12Resource& d3dResource() { return *m_data.resource; }
    virtual void init();

    const Dx12ResourceConfig& config() const { return m_config; }
    
    D3D12_GPU_VIRTUAL_ADDRESS gpuVA() const { return m_data.gpuVirtualAddress; }
    void* mappedMemory();

protected:
    //Dx12RDMgr::Handle m_srvHandle;
    //Dx12RDMgr::Handle m_uavHandle;

    struct Data
    {
        D3D12_HEAP_PROPERTIES heapProps;
        D3D12_RESOURCE_DESC resDesc;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        D3D12_HEAP_FLAGS heapFlags;
        ID3D12Resource* resource;
        UINT64 memoryOffset;
        const ID3D12Resource* externResource;
        void* mappedMemory;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
    } m_data;

    bool m_ownsResource = true;
    bool m_resolveGpuAddress = true;

    D3D12_RESOURCE_STATES m_defaultState = D3D12_RESOURCE_STATE_COMMON;
    Dx12ResourceConfig m_config;
    Dx12Device& m_device;
};


}
}
