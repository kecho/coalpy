#include "ExrCodec.h"

#include <coalpy.core/Assert.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfStringAttribute.h>
#include <ImfIO.h>

#include <IexExport.h>
#include <IexBaseExc.h>
#include <IexMathExc.h>
#include <half.h>
#include <halfExport.h>
#include <IexThrowErrnoExc.h>


//HACK: this is required to compile openexr 2.5.7 in linux
#if __linux__

namespace Iex_2_5
{

DEFINE_EXC_EXP(IEX_EXPORT, E, MathExc)
DEFINE_EXC_EXP_IMPL(IEX_EXPORT, E, MathExc)

}

IEX_INTERNAL_NAMESPACE_HEADER_ENTER
IEX_EXPORT void throwErrnoExc(const std::string &txt, int errnum) {}
IEX_EXPORT void throwErrnoExc(const std::string &txt) {}
IEX_EXPORT void throwErrnoExc() {}
IEX_INTERNAL_NAMESPACE_HEADER_EXIT

const unsigned short half::_eLut[1 << 9] =
{
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,  1024,  2048,  3072,  4096,  5120,  6144,  7168, 
     8192,  9216, 10240, 11264, 12288, 13312, 14336, 15360, 
    16384, 17408, 18432, 19456, 20480, 21504, 22528, 23552, 
    24576, 25600, 26624, 27648, 28672, 29696,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0, 33792, 34816, 35840, 36864, 37888, 38912, 39936, 
    40960, 41984, 43008, 44032, 45056, 46080, 47104, 48128, 
    49152, 50176, 51200, 52224, 53248, 54272, 55296, 56320, 
    57344, 58368, 59392, 60416, 61440, 62464,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
        0,     0,     0,     0,     0,     0,     0,     0, 
};

HALF_EXPORT const half::uif half::_toFloat[1 << 16] =
    #include "toFloat.h"

#endif

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

ImgCodecResult ExrCodec::decompress(const unsigned char* buffer, size_t bufferSize, IImgImporter& outData)
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
        int pixels = imageSize.x * imageSize.y;
        int channelCount = (rChannel != nullptr ? 1 : 0) + (gChannel != nullptr ? 1 : 0) + (bChannel != nullptr ? 1 : 0) + (aChannel != nullptr ? 1 : 0);

        Imf::FrameBuffer fb;
        if (channelCount == 4)
        {
            char* data = (char*)outData.allocate(ImgColorFmt::Rgba32, imageSize.x, imageSize.y, sizeof(float) * 4 * pixels);
            fb.insert("R", Imf::Slice(Imf::FLOAT, data                    , 4 * sizeof(float), 4 * sizeof(float) * imageSize.x));
            fb.insert("G", Imf::Slice(Imf::FLOAT, data + 1 * sizeof(float), 4 * sizeof(float), 4 * sizeof(float) * imageSize.x));
            fb.insert("B", Imf::Slice(Imf::FLOAT, data + 2 * sizeof(float), 4 * sizeof(float), 4 * sizeof(float) * imageSize.x));
            fb.insert("A", Imf::Slice(Imf::FLOAT, data + 3 * sizeof(float), 4 * sizeof(float), 4 * sizeof(float) * imageSize.x));
        }
        else if (channelCount == 3)
        {
            if (!rChannel || !bChannel || !gChannel)
                return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format with 3 channels must have channel R G and B" };

            char* data = (char*)outData.allocate(ImgColorFmt::Rgb32, imageSize.x, imageSize.y, sizeof(float) * 3 * pixels);
            fb.insert("R", Imf::Slice(Imf::FLOAT, data                    , 3 * sizeof(float), 3 * sizeof(float) * imageSize.x));
            fb.insert("G", Imf::Slice(Imf::FLOAT, data + 1 * sizeof(float), 3 * sizeof(float), 3 * sizeof(float) * imageSize.x));
            fb.insert("B", Imf::Slice(Imf::FLOAT, data + 2 * sizeof(float), 3 * sizeof(float), 3 * sizeof(float) * imageSize.x));
        }
        else if (channelCount == 2)
        {
            if (!rChannel || !gChannel)
                return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format with 2 channel must have channel R and G" };
            char* data = (char*)outData.allocate(ImgColorFmt::Rg32, imageSize.x, imageSize.y, sizeof(float) * 2 * pixels);
            fb.insert("R", Imf::Slice(Imf::FLOAT, data                     , 2 * sizeof(float), 2 * sizeof(float) * imageSize.x));
            fb.insert("G", Imf::Slice(Imf::FLOAT, data +  1 * sizeof(float), 2 * sizeof(float), 2 * sizeof(float) * imageSize.x));
        }
        else if (channelCount == 1)
        {
            if (!rChannel)
                return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format with 1 channel must have channel R" };
            char* data = (char*)outData.allocate(ImgColorFmt::R32, imageSize.x, imageSize.y, sizeof(float) * pixels);
            fb.insert("R", Imf::Slice(Imf::FLOAT, data , sizeof(float), sizeof(float) * imageSize.x));
        }
        else
            return ImgCodecResult{ TextureStatus::CorruptedFile, "EXR format must have channels R,RGB or RGBA" };

        inputFile.setFrameBuffer(fb);
        inputFile.readPixels(0, imageSize.y - 1);
    }
    catch (const std::exception& exc)
    {
        std::stringstream ss;
        ss << "Exception when reading EXR " << exc.what();
        return ImgCodecResult{ TextureStatus::CorruptedFile, ss.str() };
    }
    return ImgCodecResult { TextureStatus::Ok };
}

}
