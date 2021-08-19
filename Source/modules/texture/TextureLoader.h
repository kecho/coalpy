#pragma once
#include <coalpy.texture/ITextureLoader.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.render/Resources.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.files/Utils.h>
#include <coalpy.core/ByteBuffer.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>

namespace coalpy
{

class GpuImageImporterShaders;

enum class ImgFmt
{
    Jpeg, Png, Exr, Count
};

enum class ImgColorFmt
{
    R, Rgba, sRgb, sRgba, R32, Rg32, Rgb32, Rgba32
};

class IImgImporter
{
public:
    virtual unsigned char* allocate(ImgColorFmt fmt, int width, int height, int bytes) = 0;
    virtual void clean() = 0;
    virtual ~IImgImporter() {}
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
        IImgImporter& outData) = 0;
};

class TextureLoader : public ITextureLoader, public IFileWatchListener
{
public:
    TextureLoader(const TextureLoaderDesc& desc);
    virtual ~TextureLoader();

    virtual void start() override;
    virtual void addPath(const char* path) override;
    virtual TextureLoadResult loadTexture(const char* fileName) override;
    virtual void unloadTexture(render::Texture texture) override;
    virtual void processTextures() override;
    virtual void onFilesChanged(const std::set<std::string>& filesChanged) override;

private:
    TextureLoadResult loadTextureInternal(const char* fileName, render::Texture existingTexture = render::Texture());

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
        std::string fileName;
        std::string resolvedFileName;
        ByteBuffer fileBuffer;
        TextureLoadResult loadResult;
        AsyncFileHandle fileHandle;
        render::Texture texture;
        IImgImporter* imageImporter = nullptr;
        IImgCodec* codec = nullptr;

        void clear()
        {
            fileName.clear();
            resolvedFileName.clear();
            fileBuffer.resize(0u);
            loadResult = TextureLoadResult();
            fileHandle = AsyncFileHandle();
            codec = nullptr;
            texture = render::Texture();
            imageImporter->clean();
        }
    };

    void cleanState(LoadingState& state, bool sync);

    LoadingState* allocateLoadState();
    void freeLoadState(LoadingState* loadState);

    std::mutex m_loadStateMutex;
    std::vector<LoadingState*> m_freeLoaderStates;
    std::unordered_map<render::Texture, LoadingState*> m_loadingStates;

    std::mutex m_completeStatesMutex;
    std::queue<LoadingState*>  m_completeStates;

    void trackTexture(const std::string& resolvedFile, render::Texture texture);
    using FileTextureMap = std::unordered_map<FileLookup, render::Texture>;
    std::mutex m_trackedFilesMutex;
    FileTextureMap m_filesToTextures;

    IFileWatcher* m_fw;
    GpuImageImporterShaders* m_imageImporterShaders = nullptr;
};

}

