#include "TextureLoader.h"

namespace coalpy
{

TextureLoader::TextureLoader(const TextureLoaderDesc& desc)
: m_ts(desc.ts)
, m_fs(desc.fs)
, m_device(desc.device)
{
}

TextureLoader::~TextureLoader()
{
}

TextureResult TextureLoader::loadTexture(const char* fileName)
{
    return TextureResult { TextureStatus::FileNotFound };
}

void TextureLoader::processTextures()
{
}

IImgCodec* findCodec(const std::string& fileName)
{
    return nullptr;
}

ITextureLoader* ITextureLoader::create(const TextureLoaderDesc& desc)
{
    return new TextureLoader(desc);
}

}
