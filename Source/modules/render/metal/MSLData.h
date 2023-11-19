#pragma once

#include <string>

namespace coalpy
{

class MSLData
{
public:
    MSLData()
        : m_refCount(0), m_valid(false)
    {
    }

    ~MSLData();

    void AddRef();
    void Release();

    std::string mslSource;
    std::string mainFn;
private:
    bool m_valid;
    int m_refCount;
};

}
