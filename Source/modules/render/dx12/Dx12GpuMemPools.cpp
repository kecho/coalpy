#include <Config.h>

#if ENABLE_DX12

#include "Dx12GpuMemPools.h"
#include "Dx12Fence.h"
#include "Dx12Utils.h"
#include <algorithm>
#include <queue>

namespace coalpy
{
namespace render
{

template<class AllocDesc, class AllocationHandle, class HeapType, class GpuAllocatorType>
class Dx12GenericResourcePool
{
public:
	Dx12GenericResourcePool(Dx12Device& device, ID3D12CommandQueue& queue, GpuAllocatorType& allocator)
	: m_device(device)
	, m_fence(*(new Dx12Fence(device, queue)))
	, m_nextFenceVal(0ull)
	, m_allocator(allocator)
	{
	}

	~Dx12GenericResourcePool()
	{
		for (auto& slot : m_heaps)
		{
			if (!slot.ranges.empty())
				m_fence.waitOnCpu(slot.ranges.front().fenceValue);

			m_allocator.destroyHeap(slot.heap);
		}
		delete &m_fence;
	}

	void beginUsage()
	{
		m_nextFenceVal++;
	}

	void endUsage()
	{
		uint64_t currFenceValue = m_fence.value();
		for (HeapSlot& slot : m_heaps)
		{
			while (!slot.ranges.empty())
			{
                if (slot.ranges.front().fenceValue >= m_fence.value())
                    break;

				slot.capacity += slot.ranges.front().size;
				slot.ranges.pop();
				if (slot.ranges.empty())
				{
					CPY_ASSERT(slot.capacity == slot.size);
					slot.offset = 0ull;
				}
			}
		}
		m_fence.signal(m_nextFenceVal);
	}

	AllocationHandle allocate(const AllocDesc& desc)
	{
		AllocationHandle h;
		if (!internalFindAlloc(desc, h))
			internalCreateNew(desc, h);
		return h;
	}

protected:
	struct Range
	{
		uint64_t fenceValue = 0ull;
		uint64_t offset = 0ull;
		uint64_t size = 0ull;
	};

	struct HeapSlot
	{
		std::queue<Range> ranges;
		uint64_t capacity = 0u;
		uint64_t size = 0ull;
		uint64_t offset = 0ull;
		HeapType heap;
	};

	void commmitRange(HeapSlot& slot, const Range& range)
	{
		if (slot.ranges.empty() || slot.ranges.back().fenceValue < m_nextFenceVal)
			slot.ranges.push(range);
		else
		{
			slot.ranges.back().size += range.size;
		}

		CPY_ASSERT(range.size <= slot.capacity);
		slot.capacity -= range.size;
		slot.offset = (slot.offset + range.size) % slot.size;
	}

	void internalCreateNew(const AllocDesc& desc, AllocationHandle& outHandle)
	{
		m_heaps.emplace_back();
		HeapSlot& newSlot = m_heaps.back();
		newSlot.heap = m_allocator.createNewHeap(desc, newSlot.size);
		newSlot.capacity = newSlot.size;
		Range range = {};

		bool rangeResult = calculateRange(desc, newSlot, range);
		(void)rangeResult;
		CPY_ASSERT_MSG(rangeResult, "Range result must not fail");

		outHandle = m_allocator.allocateHandle(desc, range.offset, newSlot.heap);
		commmitRange(newSlot, range);
	}

	bool calculateRange(const AllocDesc& desc, const HeapSlot& slot, Range& outRange)
	{
		outRange = {};
		outRange.fenceValue = m_nextFenceVal;
		m_allocator.getRange(desc, slot.offset, outRange.offset, outRange.size);
		if (outRange.offset >= slot.size)
			return false;

		CPY_ASSERT(outRange.offset >= slot.offset);
		uint64_t padding = outRange.offset - slot.offset;
		uint64_t sizeLeft = slot.size - outRange.offset;

		if (outRange.size > sizeLeft)
		{
			padding += sizeLeft;
			outRange.offset = 0ull;
		}

		if ((outRange.size + padding) > slot.capacity)
			return false;

		outRange.size += padding;

		return true;
	}

	bool internalFindAlloc(const AllocDesc& desc, AllocationHandle& outHandle)
	{
		for (HeapSlot& slot : m_heaps)
		{
			if (slot.capacity == 0u)
				continue;

			Range range = {};
			if (calculateRange(desc, slot, range))
			{
				outHandle = m_allocator.allocateHandle(desc, range.offset, slot.heap);
				commmitRange(slot, range);
				return true;
			}
		}
		return false;
	}

	std::vector<HeapSlot> m_heaps;
	GpuAllocatorType& m_allocator;

	uint64_t m_nextFenceVal;
    Dx12Device& m_device;
	Dx12Fence& m_fence;
};

struct Dx12UploadDesc
{
	uint64_t alignment = 0ull;
	uint64_t requestBytes = 0ull;
};

struct Dx12UploadHeap
{
	ID3D12Heap* heap;
	ID3D12Resource* buffer;
	void* mappedMemory = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuHeapBaseVA = 0;
	uint64_t size = 0;
};

template<class GpuAllocatorType>
using BaseGpuUploadPool = Dx12GenericResourcePool<Dx12UploadDesc, Dx12GpuMemoryBlock, Dx12UploadHeap, GpuAllocatorType>;
class Dx12GpuUploadPoolImpl : public BaseGpuUploadPool<Dx12GpuUploadPoolImpl>
{
public:
	typedef BaseGpuUploadPool<Dx12GpuUploadPoolImpl> SuperType;

	Dx12GpuUploadPoolImpl(Dx12Device& device, ID3D12CommandQueue& queue, uint64_t initialHeapSize)
	: SuperType(device, queue, *this), m_nextHeapSize(initialHeapSize)
	{
	}

	~Dx12GpuUploadPoolImpl()
	{
	}

	Dx12UploadHeap createNewHeap(const Dx12UploadDesc& desc, uint64_t& outHeapSize);
	void getRange(const Dx12UploadDesc& desc, uint64_t inputOffset, uint64_t& outputOffset, uint64_t& outputSize) const;
	Dx12GpuMemoryBlock allocateHandle(const Dx12UploadDesc& desc, uint64_t heapOffset, const Dx12UploadHeap& heap);
	void destroyHeap(Dx12UploadHeap& heap);
private:
	uint64_t m_nextHeapSize;
};

struct Dx12DescriptorTableDesc
{
	uint32_t tableSize = 0u;
};

struct Dx12DescriptorTableHeap
{
	uint32_t descriptorCounts = 0u;
	ID3D12DescriptorHeap* heap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart = {};
};

template<class GpuAllocatorType>
using BaseResourceDescriptorPool = Dx12GenericResourcePool<Dx12DescriptorTableDesc, Dx12GpuDescriptorTable, Dx12DescriptorTableHeap, GpuAllocatorType>;
class Dx12GpuDescriptorTablePoolImpl : public BaseResourceDescriptorPool<Dx12GpuDescriptorTablePoolImpl>
{
public:
	typedef BaseResourceDescriptorPool<Dx12GpuDescriptorTablePoolImpl> SuperType;
	Dx12GpuDescriptorTablePoolImpl(Dx12Device& device, ID3D12CommandQueue& queue, uint32_t maxDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	    : SuperType(device, queue, *this)
	    , m_maxDescriptorCount(maxDescriptorCount)
	    , m_heapType(heapType)
	{
		m_descriptorSize = m_device.device().GetDescriptorHandleIncrementSize(heapType);
	}

	~Dx12GpuDescriptorTablePoolImpl()
	{
	}

	Dx12DescriptorTableHeap createNewHeap(const Dx12DescriptorTableDesc& desc, uint64_t& outHeapSize);
	void getRange(const Dx12DescriptorTableDesc& desc, uint64_t inputOffset, uint64_t& outputOffset, uint64_t& outputSize) const;
	Dx12GpuDescriptorTable allocateHandle(const Dx12DescriptorTableDesc& desc, uint64_t heapOffset, const Dx12DescriptorTableHeap& heap);
	void destroyHeap(Dx12DescriptorTableHeap& heap);

private:
	uint32_t m_descriptorSize;
	uint32_t m_maxDescriptorCount;
	D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
};


Dx12UploadHeap Dx12GpuUploadPoolImpl::createNewHeap(const Dx12UploadDesc& desc, uint64_t& outHeapSize)
{
	Dx12UploadHeap outHeap = {};

	D3D12_HEAP_DESC heapDesc = {};
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	heapDesc.SizeInBytes = max(2*desc.requestBytes, m_nextHeapSize);
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	m_nextHeapSize = max(2*desc.requestBytes, 2 * m_nextHeapSize);
	
#if FLUX_CHECK_MEMORY_USAGE_ON_ALLOCATION
//	DXGI_QUERY_VIDEO_MEMORY_INFO memInfo{};
//	m_dxgiAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);
//	CPY_ASSERT_MSG(memInfo.CurrentUsage + desc.SizeInBytes <= memInfo.Budget,
//		"Current memory budget is %u and current usage is %u, cannot Allocate %u more bytes.",
//		memInfo.Budget, memInfo.CurrentUsage, desc.SizeInBytes);
#endif

	DX_OK(m_device.device().CreateHeap(&heapDesc, DX_RET(outHeap.heap)));

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = heapDesc.SizeInBytes;
	resourceDesc.Height = resourceDesc.SampleDesc.Count = resourceDesc.DepthOrArraySize = resourceDesc.MipLevels = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	DX_OK(m_device.device().CreatePlacedResource(outHeap.heap, 0, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, DX_RET(outHeap.buffer)));
	outHeap.size = heapDesc.SizeInBytes;
	outHeap.gpuHeapBaseVA = outHeap.buffer->GetGPUVirtualAddress();
	outHeapSize = outHeap.size;
	DX_OK(outHeap.buffer->Map(0, nullptr, &outHeap.mappedMemory));
	return outHeap;
}

void Dx12GpuUploadPoolImpl::getRange(const Dx12UploadDesc& desc, uint64_t inputOffset, uint64_t& outOffset, uint64_t& outSize) const
{
	outSize = alignByte(desc.requestBytes, desc.alignment);
	outOffset = alignByte(inputOffset, desc.alignment);
}

Dx12GpuMemoryBlock Dx12GpuUploadPoolImpl::allocateHandle(const Dx12UploadDesc& desc, uint64_t heapOffset, const Dx12UploadHeap& heap)
{
	CPY_ASSERT((heapOffset + desc.requestBytes) <= heap.size);
	CPY_ASSERT((heapOffset % desc.alignment) == 0u);

	Dx12GpuMemoryBlock block = {};
	block.uploadSize = (size_t)alignByte(desc.requestBytes, desc.alignment);
	block.buffer = heap.buffer;
	block.mappedBuffer = static_cast<void*>(static_cast<char*>(heap.mappedMemory) + heapOffset);
	block.gpuVA  = heap.gpuHeapBaseVA + heapOffset;
	block.offset = heapOffset;
	return block;
}

void Dx12GpuUploadPoolImpl::destroyHeap(Dx12UploadHeap& heap)
{
	if (heap.buffer)
	{
		heap.buffer->Unmap(0u, nullptr);
		heap.buffer->Release();
	}

	if (heap.heap)
        heap.heap->Release();
}

Dx12DescriptorTableHeap Dx12GpuDescriptorTablePoolImpl::createNewHeap(const Dx12DescriptorTableDesc& desc, uint64_t& outHeapSize)
{
	uint32_t heapTargetSize = max(m_maxDescriptorCount, desc.tableSize);
	Dx12DescriptorTableHeap outHeap{};
	outHeapSize = heapTargetSize;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = heapTargetSize;
	heapDesc.Type = m_heapType;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX_OK(m_device.device().CreateDescriptorHeap(&heapDesc, DX_RET(outHeap.heap)));

	outHeap.cpuHandleStart = outHeap.heap->GetCPUDescriptorHandleForHeapStart();
	outHeap.gpuHandleStart = outHeap.heap->GetGPUDescriptorHandleForHeapStart();
	outHeap.descriptorCounts = heapTargetSize;

	return outHeap;
}

void Dx12GpuDescriptorTablePoolImpl::getRange(const Dx12DescriptorTableDesc& desc, uint64_t inputOffset, uint64_t& outputOffset, uint64_t& outputSize) const
{ 
	outputOffset = inputOffset; //no alignment requirements
	outputSize = desc.tableSize;
}

Dx12GpuDescriptorTable Dx12GpuDescriptorTablePoolImpl::allocateHandle(const Dx12DescriptorTableDesc& desc, uint64_t heapOffset, const Dx12DescriptorTableHeap& heap)
{
	CPY_ASSERT(heapOffset < heap.descriptorCounts);
	CPY_ASSERT(desc.tableSize <= (heap.descriptorCounts - heapOffset));
	Dx12GpuDescriptorTable outTable{};
	outTable.cpuHandle = { heap.cpuHandleStart.ptr + (uint32_t)heapOffset * m_descriptorSize };
	outTable.gpuHandle = { heap.gpuHandleStart.ptr + (uint32_t)heapOffset * m_descriptorSize };
	outTable.ownerHeap = heap.heap;
	outTable.descriptorCounts = desc.tableSize;
	outTable.descriptorSize = m_descriptorSize;
	return outTable;
}

void Dx12GpuDescriptorTablePoolImpl::destroyHeap(Dx12DescriptorTableHeap& heap)
{
	if (heap.heap)
        heap.heap->Release();
}

Dx12GpuUploadPool::Dx12GpuUploadPool(Dx12Device& device, ID3D12CommandQueue& queue, uint64_t initialPoolSize)
{
	m_impl = new Dx12GpuUploadPoolImpl(device, queue, initialPoolSize);
}

void Dx12GpuUploadPool::beginUsage()
{
	m_impl->beginUsage();
}

void Dx12GpuUploadPool::endUsage()
{
	m_impl->endUsage();
}

Dx12GpuUploadPool::~Dx12GpuUploadPool()
{
	delete m_impl;
}

Dx12GpuMemoryBlock Dx12GpuUploadPool::allocUploadBlock(size_t sizeBytes)
{
	Dx12UploadDesc desc;
	desc.alignment = 256ull;
	desc.requestBytes = sizeBytes;
	return m_impl->allocate(desc);
}

Dx12GpuDescriptorTablePool::Dx12GpuDescriptorTablePool(Dx12Device& device, ID3D12CommandQueue& queue, uint32_t maxDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	m_impl = new Dx12GpuDescriptorTablePoolImpl(device, queue, maxDescriptorCount, heapType);
}

Dx12GpuDescriptorTablePool::~Dx12GpuDescriptorTablePool()
{
	delete m_impl;
}

void Dx12GpuDescriptorTablePool::beginUsage()
{
	m_impl->beginUsage();
}

void Dx12GpuDescriptorTablePool::endUsage()
{
	m_impl->endUsage();
}

Dx12GpuDescriptorTable Dx12GpuDescriptorTablePool::allocateTable(uint32_t tableSize)
{
	CPY_ASSERT(tableSize > 0u);
	Dx12DescriptorTableDesc desc;
	desc.tableSize = tableSize;
	Dx12GpuDescriptorTable result = m_impl->allocate(desc);
	m_lastTable = result;
	return result;
}

}
}

#endif
