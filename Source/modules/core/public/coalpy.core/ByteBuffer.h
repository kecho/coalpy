#pragma once

namespace coalpy
{

typedef unsigned char u8;

class ByteBuffer
{
public:
    ByteBuffer();
    ByteBuffer(const u8* data, size_t size);
    ByteBuffer(size_t size);
    ByteBuffer(ByteBuffer&& other);
    ~ByteBuffer();
    
    u8* data() { return m_data; }
    const u8* data() const { return m_data; }
    size_t size() const { return m_size; }

    template<typename StructType>
    void append(StructType* t)
    {
        append((const u8*)t, sizeof(StructType));
    }

    void append(const u8* data, size_t size);
    inline void appendEmpty(size_t size) { append(nullptr, size); }
    void resize(size_t newSize);
    void free();
    void forget();
    void reserve(size_t newCapacity);

    ByteBuffer& operator=(ByteBuffer&& other);

private:
    u8* m_data;
    size_t m_size;
    size_t m_capacity;
};

}
