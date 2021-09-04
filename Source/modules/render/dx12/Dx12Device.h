#pragma once

#ifndef INCLUDED_T_DEVICE_H
#include <TDevice.h>
#endif

#include <coalpy.render/Resources.h>
#include <unordered_map>

struct IDXGIFactory2;
struct ID3D12Device2;
struct ID3D12Debug;
struct ID3D12RootSignature; 
struct ID3D12Pageable;

namespace coalpy
{

class Dx12ShaderDb;

namespace render
{

class Dx12Queues;
class Dx12DescriptorPool;
class Dx12ResourceCollection;
class Dx12BufferPool;
class Dx12CounterPool;
class Dx12Gc;
struct Dx12WorkInformationMap;

class Dx12Device : public TDevice<Dx12Device>
{
public:
    enum : int
    {
        MaxNumTables = 8
    };

    enum class TableTypes : int
    {
        Srv, Uav, Cbv, Sampler, Count
    };

    static IDXGIFactory2* dxgiFactory();

    Dx12Device(const DeviceConfig& config);
    virtual ~Dx12Device();

    static void enumerate(std::vector<DeviceInfo>& outputList);

    virtual TextureResult createTexture(const TextureDesc& desc) override;
    virtual TextureResult recreateTexture(Texture texture, const TextureDesc& desc) override;
    virtual BufferResult  createBuffer (const BufferDesc& config) override;
    virtual SamplerResult createSampler (const SamplerDesc& config) override;
    virtual InResourceTableResult createInResourceTable  (const ResourceTableDesc& config) override;
    virtual OutResourceTableResult createOutResourceTable (const ResourceTableDesc& config) override;
    virtual SamplerTableResult  createSamplerTable (const ResourceTableDesc& config) override;
    virtual void getResourceMemoryInfo(ResourceHandle handle, ResourceMemoryInfo& memInfo) override;
    virtual WaitStatus waitOnCpu(WorkHandle handle, int milliseconds = 0) override;
    virtual DownloadStatus getDownloadStatus(WorkHandle bundle, ResourceHandle handle) override;
    virtual void release(ResourceHandle resource) override;
    virtual void release(ResourceTable table) override;
    virtual const DeviceInfo& info() const override { return m_info; }
    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) override;

    virtual IShaderDb* db() override { return (IShaderDb*)m_shaderDb; }

    void internalReleaseWorkHandle(WorkHandle handle);

    ID3D12Device2& device() { return *m_device; }
    Dx12Queues& queues() { return *m_queues; }
    Dx12ResourceCollection& resources() { return *m_resources; }
    Dx12CounterPool& counterPool() { return *m_counterPool; }
    Dx12DescriptorPool& descriptors() { return *m_descriptors; }
    Dx12ShaderDb& shaderDb() { return *m_shaderDb; }
    Dx12BufferPool& readbackPool() { return *m_readbackPool; }
    WorkBundleDb& workDb() { return m_workDb; } 


    ID3D12RootSignature& defaultComputeRootSignature() const { return *m_computeRootSignature; }

    int tableIndex(TableTypes type, int registerSpace) const
    {
        CPY_ASSERT(registerSpace < (int)Dx12Device::MaxNumTables);
        return (int)type * (int)MaxNumTables + (registerSpace & 0x7);
    }

    ScheduleStatus internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle); 

    void deferRelease(ID3D12Pageable& object);

private:
    ID3D12Debug* m_debugLayer;
    ID3D12Device2* m_device;
    ID3D12RootSignature* m_computeRootSignature;
    Dx12CounterPool* m_counterPool;
    Dx12ResourceCollection* m_resources;
    Dx12DescriptorPool* m_descriptors;
    Dx12Queues* m_queues;
    Dx12BufferPool* m_readbackPool;
    Dx12ShaderDb* m_shaderDb;
    Dx12Gc* m_gc;
    DeviceInfo m_info;

    Dx12WorkInformationMap* m_dx12WorkInfos;
};

}
}

