#pragma once

#include <coalpy.render/Resources.h>
#include <string>

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
    InvalidExtension,
    InvalidArguments,
    JpegCodecError,
    JpegFormatNotSupported,
    PngDecompressError,
    CorruptedFile,
    Ok
};

struct TextureLoadResult
{
    TextureStatus result = TextureStatus::Ok;
    bool success() const { return result == TextureStatus::Ok; }
    render::Texture texture;
    std::string message;
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
    static ITextureLoader* create(const TextureLoaderDesc& desc);

    virtual ~ITextureLoader(){}
    virtual void start() = 0;
    virtual void addPath(const char* path) = 0;
    virtual TextureLoadResult loadTexture(const char* fileName) = 0;
    virtual void processTextures() = 0;
};

}
