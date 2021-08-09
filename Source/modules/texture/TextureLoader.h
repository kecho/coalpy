#pragma once

#include <coalpy.texture/ITextureLoader.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.render/Resources.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.core/ByteBuffer.h>
#include <vector>
#include <string>
#include <queue>
#include <mutex>

namespace coalpy
{

enum class ImgFmt
{
    Jpeg, Png, Count
};

enum class ImgColorFmt
{
    R, Rgba, sRgb, sRgba
};

class IImgData
{
public:
    virtual unsigned char* allocate(ImgColorFmt fmt, int width, int height, int bytes) = 0;
    virtual void clean() = 0;
    virtual ~IImgData() {}
};

struct ImgCodecResult
{
    TextureStatus status = TextureStatus::Ok;
    bool success() const { return status == TextureStatus::Ok; }
    std::string message;
};

class IImgCodec
{
public:
    virtual ImgFmt format() const = 0;
    virtual ImgCodecResult decompress(
        const unsigned char* buffer,
        size_t bufferSize,
        IImgData& outData) = 0;
};

class TextureLoader : public ITextureLoader
{
public:
    TextureLoader(const TextureLoaderDesc& desc);
    virtual ~TextureLoader();

    virtual void start() override;
    virtual void addPath(const char* path) override;
    virtual TextureLoadResult loadTexture(const char* fileName) override;
    virtual void processTextures() override;

private:

    IImgCodec* findCodec(const std::string& fileName);
    ITaskSystem* m_ts = nullptr;
    IFileSystem* m_fs = nullptr;
    render::IDevice* m_device = nullptr;
    std::vector<std::string> m_additionalPaths;
    
    IImgCodec* m_codecs[(int)ImgFmt::Count];

    ShaderHandle m_srgbToSrgba;

    struct LoadingState
    {
        bool loadSuccess = false;
        ByteBuffer fileBuffer;
        TextureLoadResult loadResult;
        AsyncFileHandle fileHandle;
        render::Texture texture;
        IImgData* imageData = nullptr;
        IImgCodec* codec = nullptr;

        void clear()
        {
            fileBuffer.resize(0u);
            loadResult = TextureLoadResult();
            fileHandle = AsyncFileHandle();
            codec = nullptr;
            texture = render::Texture();
            imageData->clean();
        }
    };

    LoadingState* allocateLoadState();
    void freeLoadState(LoadingState* loadState);

    std::vector<LoadingState*> m_freeLoaderStates;

    std::mutex m_completeStatesMutex;
    std::queue<LoadingState*>  m_completeStates;
};

}

