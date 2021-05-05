#pragma once
#include <vector>
#include <functional>

namespace coalpy
{

template<typename HandleType, typename DataType>
class HandleContainer
{
public:
    using OnElementFn = std::function<void(HandleType handle, DataType& data)>;

    DataType& allocate(HandleType& outHandle)
    {
        if (m_freeHandles.empty())
        {
            outHandle = (HandleType::BaseType)m_data.size();
        }
        else
        {
            outHandle = m_freeHandles.back();
            m_freeHandles.pop_back();
        }
        auto& container = m_data[handle];
        container.handle = handle; 
        container.valid = true;
        return container.data;
    }

    void free(HandleType handle, bool resetObject = true)
    {
        if (handle >= (HandleType)(HandleType::BaseType)m_data.size())
            return;

        m_freeHandles.push_back(handle);
        m_data[handle].valid = false;
        if (resetObject)
            m_data[handle].data = DataType();
    }

    void forEach(OnElementFn elementfn)
    {
        for (DataContainer& c : m_data)
        {
            if (c.valid)
                elementfn(c.handle, c.data);
        }
    }

private:
    struct DataContainer
    {
        HandleType handle;
        bool valid = false;
        DataType data;
    };

    std::vector<HandleType> m_freeHandles;
    std::vector<DataContainer> m_data;
};

}
