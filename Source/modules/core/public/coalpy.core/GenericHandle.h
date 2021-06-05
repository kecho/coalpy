#pragma once

namespace coalpy
{

template <typename BaseHandle>
struct GenericHandle
{
    typedef BaseHandle BaseType;
    enum : BaseHandle { InvalidId = (BaseHandle)~0 };
    BaseHandle handleId;

    GenericHandle()
    {
        handleId = InvalidId;
    }

    GenericHandle(BaseHandle inHandleId)
    {
        handleId = inHandleId;
    }

    GenericHandle(const GenericHandle& other)
    {
        *this = other;
    }

    bool valid() const
    {
        return handleId != InvalidId;
    }

    GenericHandle& operator=(const GenericHandle& other)
    {
        handleId = other.handleId;
        return *this;
    }

    bool operator==(const GenericHandle<BaseHandle>& other) const
    {
        return handleId == other.handleId;
    }
    
    bool operator!=(const GenericHandle<BaseHandle>& other) const
    {
        return !((*this) == other);
    }

    bool operator< (const GenericHandle<BaseHandle>& other) const
    {
        return handleId < other.handleId;
    }

    bool operator<=(const GenericHandle<BaseHandle>& other) const
    {
        return handleId <= other.handleId;
    }

    bool operator> (const GenericHandle<BaseHandle>& other) const
    {
        return handleId > other.handleId;
    }

    bool operator>= (const GenericHandle<BaseHandle>& other) const
    {
        return handleId >= other.handleId;
    }

    operator BaseHandle() const { return handleId; }
};

}
