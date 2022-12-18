#include <Config.h>
#include <queue>
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

// This is a generic base class for a circular buffer pool that is live on the GPU.
// This class syncrhonizes uploads to the GPU or allocations utilizing a fence.
// The beginUsage and endUsage should be called between execution. Internally, these functions
// perform fence checks on the GPU to ensure that we can synchronize correctly.
template<
    class AllocDesc,
    class AllocationHandle,
    class HeapType,
    class GpuAllocatorType,
    class FenceTimelineType>
class TGpuResourcePool
{
public:
    typedef typename FenceTimelineType::FenceType FenceType;

    TGpuResourcePool(GpuAllocatorType& allocator, FenceTimelineType& fenceTimeline)
    : m_allocator(allocator), m_fenceTimeline(fenceTimeline)
    {
    }

    ~TGpuResourcePool()
    {
        for (auto& slot : m_heaps)
        {
            if (!slot.ranges.empty())
                m_fenceTimeline.waitOnCpu(slot.ranges.front().fenceValue);

            m_allocator.destroyHeap(slot.heap);
        }
    }

    void beginUsage()
    {
    }

    void endUsage()
    {
        for (HeapSlot& slot : m_heaps)
        {
            while (!slot.ranges.empty())
            {
                if (!m_fenceTimeline.isSignaled(slot.ranges.front().fenceValue))
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
        m_fenceTimeline.signalFence();
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
        FenceType fenceValue = {};
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

    void commitRange(HeapSlot& slot, const Range& range)
    {
        if (slot.ranges.empty())
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
        commitRange(newSlot, range);
    }

    bool calculateRange(const AllocDesc& desc, const HeapSlot& slot, Range& outRange)
    {
        outRange = {};
        m_allocator.getRange(desc, slot.offset, outRange.offset, outRange.size);
        if (outRange.offset >= slot.size)
            return false;

        CPY_ASSERT(outRange.offset >= slot.offset);
        uint64_t padding = outRange.offset - slot.offset;
        uint64_t sizeLeft = slot.size - outRange.offset;

        if (outRange.size > sizeLeft)
        {
            padding += sizeLeft; outRange.offset = 0ull;
        }

        if ((outRange.size + padding) > slot.capacity)
            return false;

        outRange.fenceValue = m_fenceTimeline.allocateFenceValue();
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
                commitRange(slot, range);
                return true;
            }
        }
        return false;
    }

    std::vector<HeapSlot> m_heaps;
    GpuAllocatorType& m_allocator;
    FenceTimelineType& m_fenceTimeline;
};


}
}
