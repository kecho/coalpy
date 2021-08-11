#include "ExrCodec.h"

#include <coalpy.core/Assert.h>
#include <OpenExr/ImfChannelList.h>
#include <OpenExr/ImfInputFile.h>
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
    try
    {
        ImfByteStream stream("exrFile", buffer, bufferSize);
        Imf::InputFile inputFile(stream);
        auto* rChannel = inputFile.header().channels().findChannel("R");
        auto* gChannel = inputFile.header().channels().findChannel("G");
        auto* bChannel = inputFile.header().channels().findChannel("B");
        auto* aChannel = inputFile.header().channels().findChannel("A");
        //auto imageSize = inputFile.header().dataWindow().size();
        auto imageSize = inputFile.header().dataWindow().max;
        imageSize.x += 1;
        imageSize.y += 1;
        int channelCount = (rChannel != nullptr ? 1 : 0) + (gChannel != nullptr ? 1 : 0) + (bChannel != nullptr ? 1 : 0) + (aChannel != nullptr ? 1 : 0);
        ImgColorFmt fmt = ImgColorFmt::sRgba;
        Imf::FrameBuffer fb;
        ByteBuffer bb;
        if (channelCount == 4)
        {
            fmt = ImgColorFmt::sRgba;
        }
        else if (channelCount == 3)
        {
            if (!rChannel || !bChannel || !gChannel)
                return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format with 3 channels must have channel R G and B" };
            fmt = ImgColorFmt::sRgb;            
        }
        else if (channelCount == 1)
        {
            if (!rChannel)
                return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format with 1 channel must have channel R" };
            fmt = ImgColorFmt::R;
            bb.appendEmpty(sizeof(float) * imageSize.x * imageSize.y);
            fb.insert("R", Imf::Slice(Imf::FLOAT, (char*)bb.data(), sizeof(float), sizeof(float) * imageSize.x));
        }
        else
            return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format must have channels R,RGB or RGBA" };

        inputFile.setFrameBuffer(fb);
        inputFile.readPixels(0, imageSize.y - 1);

        if (!inputFile.isComplete())
            return ImgCodecResult{ TextureStatus::CorruptedFile, "Failed reading EXR file." };

        CPY_ASSERT(inputFile.frameBuffer()[0].base != nullptr);
    }
    catch (const std::exception& exc)
    {
        std::stringstream ss;
        ss << "Exception when reading EXR " << exc.what();
        return ImgCodecResult{ TextureStatus::CorruptedFile, ss.str() };
    }
    return ImgCodecResult{ TextureStatus::FileNotFound, "TODO: read exr" };
}

}
