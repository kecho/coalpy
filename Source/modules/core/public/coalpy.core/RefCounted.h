#pragma once

namespace coalpy
{

class RefCounted
{
public:
    virtual ~RefCounted() {}

    void AddRef()
    {
        ++m_ref;
    }

    void Release()
    {
        --m_ref;
        if (m_ref <= 0)
            delete this;
    }
    
private:
    int m_ref = 0;
};

}
