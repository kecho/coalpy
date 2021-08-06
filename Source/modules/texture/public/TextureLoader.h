#pragma once

#include <coalpy.texture/ITexutreLoader.h>

namespace coalpy
{


class TextureLoader : public ITexutreLoader
{
public:
    virtual ~TextureLoader();
    virtual TextureResult loadTexture(const char* fileName);
};


}

