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
    IImgImporter& outData)
{
    png_image image = {};
    image.version = PNG_IMAGE_VERSION;
    
    if (!png_image_begin_read_from_memory(&image, buffer, bufferSize))
    {
        std::stringstream ss;
        ss << "Png decompression error: " << image.message;
        return ImgCodecResult { TextureStatus::PngDecompressError, ss.str() };
    }

    image.format = PNG_FORMAT_RGBA;
    size_t sz = PNG_IMAGE_SIZE(image);
    CPY_ASSERT(sz == image.width * image.height * 4);
    unsigned char* imageData = outData.allocate(ImgColorFmt::Rgba, image.width, image.height, sz);
    if (!imageData)
    { 
        std::stringstream ss;
        ss << "Error allocating memory for image.";
        return ImgCodecResult { TextureStatus::CorruptedFile, ss.str() };
    }

    if (!png_image_finish_read(&image, nullptr, (void*)imageData, 0, nullptr))
    {
        std::stringstream ss;
        ss << "Png reading error. " << image.message;
        return ImgCodecResult { TextureStatus::CorruptedFile, ss.str() };
    }

    return ImgCodecResult { TextureStatus::Ok };
}

}
