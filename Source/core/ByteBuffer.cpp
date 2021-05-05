#include <coalpy.core/ByteBuffer.h>

namespace coalpy
{

ByteBuffer::ByteBuffer()
{
}

ByteBuffer::ByteBuffer(size_t size)
{
    m_support.resize(size);
}

ByteBuffer::ByteBuffer(ByteBuffer&& other)
{
    m_support = std::move(other.m_support);
}

void ByteBuffer::resize(size_t newSize)
{
    m_support.resize(newSize);
}

void ByteBuffer::free()
{
    m_support.clear();
}

}
