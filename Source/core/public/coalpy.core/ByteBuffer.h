#pragma once

namespace coalpy
{

typedef unsigned char u8;

class ByteBuffer
{
public:
    ByteBuffer();
    ByteBuffer(size_t size);
    ByteBuffer(ByteBuffer&& other);
    ~ByteBuffer();
    
    u8* data() { return m_data; }
    const u8* data() const { return m_data; }
    size_t size() const { return m_size; }

    void append(const u8* data, size_t size);
    void resize(size_t newSize);
    void free();
    void forget();
    void reserve(size_t newCapacity);

private:
    u8* m_data;
    size_t m_size;
    size_t m_capacity;
};

}
