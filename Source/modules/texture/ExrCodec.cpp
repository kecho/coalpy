#include "ExrCodec.h"

#include <OpenExr/ImfChannelList.h>
#include <OpenExr/ImfOutputFile.h>
#include <OpenExr/ImfStringAttribute.h>
#include <OpenExr/ImfIO.h>

namespace coalpy
{

class ImfByteStream : public Imf::IStream
{
public:
    ImfByteStream(const char* name, const unsigned char* buffer, size_t bufferSize)
    : Imf::IStream(name), m_buffer(buffer), m_bufferSize(bufferSize)
    {
    }

    virtual bool isMemoryMapped () const { return true; }
    
    virtual bool read (char c[/*n*/], int n)
    {
        memcpy(c, m_buffer + m_i, n);
        m_i += n;
        return m_i < m_bufferSize;
    }

    virtual char* readMemoryMapped(int n)
    {
        auto ptr = const_cast<char*>((const char*)m_buffer + m_i);
        m_i += n;
        return ptr;
    }

    
    virtual Imf::Int64 tellg () 
    {
        return m_i;
    }

    virtual void seekg (Imf::Int64 pos)
    {
        m_i = pos;
    }

    virtual ~ImfByteStream() {}

private:
    const unsigned char* m_buffer;
    size_t m_bufferSize;
    Imf::Int64 m_i = 0;
};

ImgCodecResult ExrCodec::decompress(const unsigned char* buffer, size_t bufferSize, IImgData& outData)
{
    ImfByteStream stream ("exrFile", buffer, bufferSize);
    return {};
}

}
