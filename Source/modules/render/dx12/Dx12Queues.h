#pragma once

struct ID3D12CommandQueue;

namespace coalpy
{
namespace render
{

class Dx12Device;
class Dx12Fence;

enum class WorkType
{
    Graphics,
    Compute,
    Copy,
    Count
};

class Dx12Queues
{
public:
    Dx12Queues(Dx12Device& device);
    ~Dx12Queues();

    ID3D12CommandQueue& directq() { return *(m_containers[(int)WorkType::Graphics].queue); }
    ID3D12CommandQueue& asyncq()  { return *(m_containers[(int)WorkType::Compute].queue); }
    ID3D12CommandQueue& copyq()   { return *(m_containers[(int)WorkType::Copy].queue); }

private:
    struct QueueContainer
    {
        ID3D12CommandQueue* queue = nullptr;
        Dx12Fence* fence = nullptr;
    };

    QueueContainer m_containers[(int)WorkType::Count];
    Dx12Device& m_device;
};

}
}
