#pragma once

#include <TDevice.h>
#include <coalpy.render/Resources.h>

struct IDXGIFactory2;
struct ID3D12Device2;
struct ID3D12Debug;

namespace coalpy
{
namespace render
{

class Dx12Queues;
class Dx12DescriptorPool;
class Dx12ResourceCollection;

class Dx12Device : public TDevice<Dx12Device>
{
public:
    static IDXGIFactory2* dxgiFactory();

    Dx12Device(const DeviceConfig& config);
    virtual ~Dx12Device();

    static void enumerate(std::vector<DeviceInfo>& outputList);

    virtual Texture createTexture(const TextureDesc& desc) override;
    virtual Buffer  createBuffer (const BufferDesc& config) override;
    virtual InResourceTable   createInResourceTable  (const ResourceTableDesc& config) override;
    virtual OutResourceTable  createOutResourceTable (const ResourceTableDesc& config) override;
    virtual void release(ResourceHandle resource) override;
    virtual void release(ResourceTable table) override;

    ID3D12Device2& device() { return *m_device; }
    Dx12Queues& queues() { return *m_queues; }
    Dx12ResourceCollection& resources() { return *m_resources; }
    Dx12DescriptorPool& descriptors() { return *m_descriptors; }

    virtual const DeviceInfo& info() const override { return m_info; }
    virtual SmartPtr<IDisplay> createDisplay(const DisplayConfig& config) override;

private:
    ID3D12Debug* m_debugLayer;
    ID3D12Device2* m_device;
    Dx12ResourceCollection* m_resources;
    Dx12DescriptorPool* m_descriptors;
    Dx12Queues* m_queues;
    DeviceInfo m_info;
    
};

}
}

