#pragma once
#include <vector>

namespace coalpy
{

class ByteBuffer
{
public:
    ByteBuffer();
    ByteBuffer(size_t size);
    ByteBuffer(ByteBuffer&& other);
    
    size_t size() { return m_support.size(); }
    uint8_t* data() { return m_support.data(); }
    const uint8_t* data() const { return m_support.data(); }
    void  resize(size_t newSize);
    void  free();

private:
    std::vector<uint8_t> m_support;
};

}
