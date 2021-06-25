#pragma once

#include <d3d12.h>
#include <coalpy.core/RefCounted.h>
#include <coalpy.render/Resources.h>
#include <string>
#include "Dx12DescriptorPool.h"

namespace coalpy
{
namespace render
{

class Dx12Device;

class Dx12Resource : public RefCounted
{
public:
    Dx12Resource(Dx12Device& device, const ResourceDesc& config,
        bool canDenyShaderResources = true);

    virtual ~Dx12Resource();

    D3D12_RESOURCE_STATES defaultState() const { return m_defaultState; }
    void acquireD3D12Resource(ID3D12Resource* resource);
    ID3D12Resource& d3dResource() const { return *m_data.resource; }
    ID3D12Resource& d3dResource() { return *m_data.resource; }
    virtual void init();

    const ResourceDesc& config() const { return m_config; }
    
    D3D12_GPU_VIRTUAL_ADDRESS gpuVA() const { return m_data.gpuVirtualAddress; }
    void* mappedMemory();

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

}
}
