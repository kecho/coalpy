#include <Config.h>
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

template<class AllocDesc, class AllocationHandle, class HeapType, class GpuAllocatorType, class FenceType>
class TGpuResourcePool
{
public:
    TGpuResourcePool(FenceType fence, GpuAllocatorType& allocator)
    : m_fence(fence) 
    , m_nextFenceVal(0ull)
    , m_allocator(allocator)
    {
    }

    ~TGpuResourcePool()
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
        uint64_t currFenceValue = m_fence.completedValue();
        for (HeapSlot& slot : m_heaps)
        {
            while (!slot.ranges.empty())
            {
                if (slot.ranges.front().fenceValue >= currFenceValue)
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
    FenceType& m_fence;
};


}
}
