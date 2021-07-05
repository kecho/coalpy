#pragma once

#include <d3d12.h>
#include <coalpy.core/RefCounted.h>
#include <coalpy.render/Resources.h>
#include <coalpy.core/Assert.h>
#include "Dx12DescriptorPool.h"
#include "WorkBundleDb.h"
#include <string>
#include <vector>
#include <functional>

namespace coalpy
{
namespace render
{

class Dx12Device;

inline ResourceGpuState getGpuState(D3D12_RESOURCE_STATES state)
{
    switch (state)
    {
        case D3D12_RESOURCE_STATE_COMMON:
            return ResourceGpuState::Default;
        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
            return ResourceGpuState::Uav;
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
            return ResourceGpuState::Srv;
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
            return ResourceGpuState::IndirectArgs;
        case D3D12_RESOURCE_STATE_COPY_DEST:
            return ResourceGpuState::CopyDst;
        case D3D12_RESOURCE_STATE_COPY_SOURCE:
            return ResourceGpuState::CopySrc;
        case D3D12_RESOURCE_STATE_GENERIC_READ:
            return ResourceGpuState::Cbv;
    }
    CPY_ASSERT_FMT(false, "D3d12 state used is not handled in coalpy's rendering", state);
    return ResourceGpuState::Default;
}

inline D3D12_RESOURCE_STATES getDx12GpuState(ResourceGpuState state)
{
    switch (state)
    {
        case ResourceGpuState::Default:
            return D3D12_RESOURCE_STATE_COMMON;
        case ResourceGpuState::Uav:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case ResourceGpuState::Srv:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case ResourceGpuState::IndirectArgs:
            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case ResourceGpuState::CopyDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceGpuState::CopySrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceGpuState::Cbv:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    CPY_ASSERT_FMT(false, "D3d12 state used is not handled in coalpy's rendering", state);
    return D3D12_RESOURCE_STATE_COMMON;
}

class Dx12Resource : public RefCounted
{
public:
    Dx12Resource(
        Dx12Device& device,
        const ResourceDesc& config,
        bool canDenyShaderResources = true);

    virtual ~Dx12Resource();

    D3D12_RESOURCE_STATES defaultD3d12State() const { return m_defaultState; }
    ResourceGpuState defaultGpuState() const { return getGpuState(defaultD3d12State()); }
    void acquireD3D12Resource(ID3D12Resource* resource);
    ID3D12Resource& d3dResource() const { return *m_data.resource; }
    ID3D12Resource& d3dResource() { return *m_data.resource; }
    virtual bool init();

    const ResourceDesc& config() const { return m_config; }
    
    D3D12_GPU_VIRTUAL_ADDRESS gpuVA() const { return m_data.gpuVirtualAddress; }
    void* mappedMemory();

    const Dx12Descriptor srv() const { return m_srv; }
    const Dx12Descriptor uav() const { return m_uav; }

protected:
    Dx12Descriptor m_srv = {};
    Dx12Descriptor m_uav = {};

    enum class Usage
    {
        Default, Dynamic, Readback
    };

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

    bool m_resolveGpuAddress = true;

    Usage m_usage = Usage::Default;
    D3D12_RESOURCE_STATES m_defaultState = D3D12_RESOURCE_STATE_COMMON;
    ResourceDesc m_config;
    Dx12Device& m_device;
};

class Dx12Texture : public Dx12Resource
{
public:
    Dx12Texture(Dx12Device& device, const TextureDesc& desc);
    virtual ~Dx12Texture();

    const TextureDesc& texDesc() const { return m_texDesc; }
protected:
    TextureDesc m_texDesc;
};

class Dx12Buffer : public Dx12Resource
{
public:
    Dx12Buffer(Dx12Device& device, const BufferDesc& desc);
    virtual ~Dx12Buffer();

    const BufferDesc& bufferDesc() const { return m_buffDesc; }

protected:
    BufferDesc m_buffDesc;
};

class Dx12ResourceTable : public RefCounted
{
public:
    Dx12ResourceTable(Dx12Device& device, Dx12Resource** resources, int count, bool isUav);
    ~Dx12ResourceTable();

private:
    Dx12Device& m_device;
    std::vector<SmartPtr<Dx12Resource>> m_resources;
    Dx12DescriptorTable m_cpuTable;
};

}
}
