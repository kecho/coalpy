#include <coalpy.core/ByteBuffer.h>
#include <stdlib.h>
#include <algorithm>
#include <cstring>

namespace coalpy
{

ByteBuffer::ByteBuffer()
: m_data(nullptr), m_size(0), m_capacity(0)
{
}

ByteBuffer::ByteBuffer(const u8* data, size_t size)
: m_data(nullptr), m_size(0), m_capacity(0)
{
    append(data, size);
}

ByteBuffer::ByteBuffer(size_t size)
: m_data(nullptr), m_size(0), m_capacity(0)
{
    resize(size);
}

ByteBuffer::ByteBuffer(ByteBuffer&& other)
{
    *this = std::move(other);
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& other)
{
    m_data = other.m_data;
    m_size = other.m_capacity;
    m_capacity = other.m_capacity;
    other.forget();
    return *this;
}

ByteBuffer::~ByteBuffer()
{
    free();
}

void ByteBuffer::append(const u8* data, size_t size)
{
    size_t totalNewSize = std::max(m_capacity, m_size + size);
    reserve(totalNewSize);
    if (data)
        memcpy(m_data + m_size, data, size);
    m_size += size;
}

void ByteBuffer::reserve(size_t newCapacity)
{
    if (newCapacity > m_capacity)
    {
        u8* newData = (u8*)malloc(newCapacity);
        size_t currentSize = m_size;
        if (currentSize > 0u)
            memcpy(newData, m_data, currentSize);
        free();
        m_size = currentSize;
        m_data = newData;
        m_capacity = newCapacity;
    }
}

void ByteBuffer::resize(size_t newSize)
{
    reserve(newSize);
    m_size = newSize;
}

void ByteBuffer::free()
{
    if (m_data)
        ::free(m_data);
    forget();
}

void ByteBuffer::forget()
{
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}

}
