#pragma once

namespace coalpy
{

template <typename BaseHandle>
struct GenericHandle
{
    typedef BaseHandle BaseType;
    enum : BaseHandle { InvalidId = (BaseHandle)~0 };
    BaseHandle handleId = InvalidId;

    bool valid() const
    {
        return handleId != InvalidId;
    }

    bool operator==(const GenericHandle<BaseHandle>& other) const
    {
        return handleId == other.handleId;
    }
    
    bool operator!=(const GenericHandle<BaseHandle>& other) const
    {
        return !((*this) == other)
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

    operator BaseHandle() const { return hadleId; }
};

}
