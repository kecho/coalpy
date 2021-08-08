#include "PngCodec.h"
#include <coalpy.core/Assert.h>
#include <png.h>
#include <zlib.h>
#include <sstream>

namespace coalpy
{

ImgCodecResult PngCodec::decompress(
    const unsigned char* buffer,
    size_t bufferSize,
    IImgData& outData)
{
    png_image image = {};
    image.version = PNG_IMAGE_VERSION;
    
    if (!png_image_begin_read_from_memory(&image, buffer, bufferSize))
    {
        std::stringstream ss;
        ss << "Png decompression error: " << image.message;
        return ImgCodecResult { TextureStatus::PngDecompressError, ss.str() };
    }

    image.format = PNG_FORMAT_RGB;
    size_t sz = PNG_IMAGE_SIZE(image);
    CPY_ASSERT(sz == image.width * image.height * 24);
    unsigned char* imageData = outData.allocate(ImgColorFmt::sRgb, image.width, image.height, sz);
    if (!png_image_finish_read(&image, nullptr, (void*)imageData, 0, nullptr))
    {
        std::stringstream ss;
        ss << "Png reading error. " << image.message;
        return ImgCodecResult { TextureStatus::CorruptedFile, ss.str() };
    }

    return ImgCodecResult { TextureStatus::CorruptedFile };
}

}
