#pragma once

#include "TextureLoader.h"

namespace coalpy
{

class PngCodec : public IImgCodec
{
public:
    virtual ImgFmt format() const override { return ImgFmt::Png; }
    virtual ImgCodecResult decompress(
        const unsigned char* buffer,
        size_t bufferSize,
        IImgData& outData) override;
};

}
