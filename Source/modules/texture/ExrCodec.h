#pragma once

#include "TextureLoader.h"

namespace coalpy
{

class ExrCodec : public IImgCodec
{
public:
    virtual ImgFmt format() const override { return ImgFmt::Exr; }
    virtual ImgCodecResult decompress(
        const unsigned char* buffer,
        size_t bufferSize,
        IImgData& outData) override;
};

}
