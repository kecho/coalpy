#pragma once

#include "TextureLoader.h"

namespace coalpy
{

class JpegCodec : public IImgCodec
{
public:
    virtual ImgFmt format() const override { return ImgFmt::Jpeg; }
    virtual ImgCodecResult decompress(
        const unsigned char* buffer,
        size_t bufferSize,
        IImgImporter& outData) override;
};

}
