#pragma once

namespace coalpy
{

template <typename BaseHandle>
struct GenericHandle
{
    typedef BaseHandle BaseType;
    BaseHandle handleId = ~0;

    bool valid() const
    {
        return handleId != GenericHandle<BaseHandle>::InvalidId;
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
