#pragma once

#include <coalpy.render/Resources.h>

namespace coalpy
{

namespace render
{
class IDevice;
}

class ITaskSystem;
class IFileSystem;

enum TextureStatus
{
    FileNotFound,
    InvalidArguments,
    CorruptedFile,
    Ok
};

struct TextureResult
{
    TextureStatus result = TextureResult::Ok;
    bool success() const { return result == TextureResult::Ok; }
    render::Texture texture;
};

struct TextureLoaderDesc
{
    render::IDevice* device = nullptr;
    ITaskSystem* ts = nullptr;
    IFileSystem* fs = nullptr;
};

class ITextureLoader
{
public:
    ITextureLoader* create(const TextureLoaderDesc& desc);

    virtual ~ITextureLoader(){}
    virtual TextureResult loadTexture(const char* fileName) = 0;
};

}
