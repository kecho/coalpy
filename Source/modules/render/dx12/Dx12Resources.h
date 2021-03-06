#pragma once

#include <d3d12.h>
#include <coalpy.core/RefCounted.h>
#include <coalpy.render/Resources.h>
#include <coalpy.core/Assert.h>
#include "Dx12DescriptorPool.h"
#include "Dx12CounterPool.h"
#include "WorkBundleDb.h"
#include <string>
#include <vector>
#include <functional>

namespace coalpy
{
namespace render
{

enum ResourceSpecialFlags : int
{
    ResourceSpecialFlag_None = 0,
    ResourceSpecialFlag_NoDeferDelete = 1 << 0,
    ResourceSpecialFlag_CanDenyShaderResources = 1 << 1,
    ResourceSpecialFlag_TrackTables = 1 << 2,
    ResourceSpecialFlag_CpuReadback = 1 << 3,
    ResourceSpecialFlag_CpuUpload = 1 << 4,
};

struct Dx12ResourceInitResult
{
    bool success() { return result == ResourceResult::Ok; }
    ResourceResult result;
    std::string message;
};


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
        case D3D12_RESOURCE_STATE_RENDER_TARGET:
            return ResourceGpuState::Rtv;
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
        case ResourceGpuState::CopyDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ResourceGpuState::CopySrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ResourceGpuState::IndirectArgs:
            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case ResourceGpuState::Cbv:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case ResourceGpuState::Rtv:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ResourceGpuState::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
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
        ResourceSpecialFlags specialFlags);

    virtual ~Dx12Resource();

    D3D12_RESOURCE_STATES defaultD3d12State() const { return m_defaultState; }
    ResourceGpuState defaultGpuState() const { return getGpuState(defaultD3d12State()); }
    void acquireD3D12Resource(ID3D12Resource* resource);
    const D3D12_RESOURCE_DESC& d3dResDesc() const { return m_data.resDesc; }
    ID3D12Resource& d3dResource() const { return *m_data.resource; }
    ID3D12Resource& d3dResource() { return *m_data.resource; }
    virtual Dx12ResourceInitResult init();

    const ResourceDesc& config() const { return m_config; }
    
    D3D12_GPU_VIRTUAL_ADDRESS gpuVA() const { return m_data.gpuVirtualAddress; }
    void* mappedMemory();

    const Dx12Descriptor srv() const { return m_srv; }
    const Dx12Descriptor uav(int mipLevel = 0) const { return m_uavs[mipLevel]; }
    int uavCounts() const { return (int)m_uavs.size(); }

    bool isBuffer() const { return m_data.resDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER; };
    ResourceSpecialFlags specialFlags() const { return m_specialFlags; }

    virtual size_t byteSize() const = 0;

    Dx12CounterHandle counterHandle() const { return m_counterHandle; }
protected:
    Dx12Descriptor m_srv = {};
    std::vector<Dx12Descriptor> m_uavs;

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
        void* mappedMemory;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
    } m_data;

    bool m_resolveGpuAddress = true;

    Usage m_usage = Usage::Default;
    ResourceSpecialFlags m_specialFlags = ResourceSpecialFlag_None;
    D3D12_RESOURCE_STATES m_defaultState = D3D12_RESOURCE_STATE_COMMON;
    ResourceDesc m_config;
    Dx12Device& m_device;
    Dx12CounterHandle m_counterHandle;
};

class Dx12Texture : public Dx12Resource
{
public:
    Dx12Texture(Dx12Device& device, const TextureDesc& desc, ResourceSpecialFlags specialFlag);
    virtual Dx12ResourceInitResult init() override;
    virtual ~Dx12Texture();

    const TextureDesc& texDesc() const { return m_texDesc; }
    virtual size_t byteSize() const;

    void getCpuTextureSizes(int mipId, int& pixelPitch, size_t& outRowPitch, int& width, int& outHeight, int& depth) const;

    int subresourceIndex(int mipLevel, int arraySlice) const;
    int mipCounts() const;
    int arraySlicesCounts() const;

protected:
    int m_slices;
    TextureDesc m_texDesc;
};

class Dx12Buffer : public Dx12Resource
{
public:
    Dx12Buffer(Dx12Device& device, const BufferDesc& desc, ResourceSpecialFlags specialFlag);
    virtual ~Dx12Buffer();

    const BufferDesc& bufferDesc() const { return m_buffDesc; }
    virtual Dx12ResourceInitResult init() override;

    const Dx12Descriptor& cbv() const { return m_cbv; }
    virtual size_t byteSize() const;

protected:
    Dx12Descriptor m_cbv = {};
    BufferDesc m_buffDesc;
};

class Dx12Sampler : public RefCounted
{
public:
    Dx12Sampler(Dx12Device& device, const SamplerDesc& desc);
    virtual ~Dx12Sampler();

    const Dx12Descriptor& descriptor() const { return m_descriptor; }

private:
    Dx12Descriptor m_descriptor;
    Dx12Device& m_device;
};

class Dx12ResourceTable : public RefCounted
{
public:
    Dx12ResourceTable(Dx12Device& device, Dx12Resource** resources, int count, bool isUav, const int* targetUavMips = nullptr);
    Dx12ResourceTable(Dx12Device& device, Dx12Sampler** samplers, int count);
    ~Dx12ResourceTable();

    const Dx12DescriptorTable& cpuTable() const { return m_cpuTable; }
private:
    Dx12Device& m_device;
    std::vector<SmartPtr<Dx12Resource>> m_resources;
    std::vector<SmartPtr<Dx12Sampler>>  m_samplers;
    Dx12DescriptorTable m_cpuTable;
};

}
}
