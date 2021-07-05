#pragma once

#include <TDevice.h>
#include <coalpy.render/Resources.h>

struct IDXGIFactory2;
struct ID3D12Device2;
struct ID3D12Debug;
struct ID3D12RootSignature; 

namespace coalpy
{

class Dx12ShaderDb;

namespace render
{

class Dx12Queues;
class Dx12DescriptorPool;
class Dx12ResourceCollection;

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

    virtual Texture createTexture(const TextureDesc& desc) override;
    virtual Buffer  createBuffer (const BufferDesc& config) override;
    virtual InResourceTable   createInResourceTable  (const ResourceTableDesc& config) override;
    virtual OutResourceTable  createOutResourceTable (const ResourceTableDesc& config) override;

    virtual WaitStatus waitOnCpu(WorkHandle handle, int milliseconds = 0) override;
    virtual DownloadStatus getDownloadStatus(WorkHandle bundle, ResourceHandle handle) override;
    virtual void release(ResourceHandle resource) override;
    virtual void release(ResourceTable table) override;

    ID3D12Device2& device() { return *m_device; }
    Dx12Queues& queues() { return *m_queues; }
    Dx12ResourceCollection& resources() { return *m_resources; }
    Dx12DescriptorPool& descriptors() { return *m_descriptors; }

    virtual const DeviceInfo& info() const override { return m_info; }
    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) override;

    ID3D12RootSignature& defaultComputeRootSignature() const { return *m_computeRootSignature; }

    int tableIndex(TableTypes type, int registerSpace) const
    {
        CPY_ASSERT(registerSpace < (int)Dx12Device::MaxNumTables);
        return (int)type * (int)TableTypes::Count + (registerSpace & 0x7);
    }

    ScheduleStatus internalSchedule(CommandList** commandLists, int listCounts, WorkHandle workHandle); 

private:
    ID3D12Debug* m_debugLayer;
    ID3D12Device2* m_device;
    ID3D12RootSignature* m_computeRootSignature;
    Dx12ResourceCollection* m_resources;
    Dx12DescriptorPool* m_descriptors;
    Dx12Queues* m_queues;
    Dx12ShaderDb* m_shaderDb;
    DeviceInfo m_info;
};

}
}

