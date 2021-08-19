#include "TextureLoader.h"
#include <coalpy.files/Utils.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/IShaderDb.h>
#include "GpuImageImporter.h"
#include "JpegCodec.h"
#include "PngCodec.h"
#include "ExrCodec.h"
#include <iostream>
#include <sstream>

namespace coalpy
{

TextureLoader::TextureLoader(const TextureLoaderDesc& desc)
: m_ts(desc.ts)
, m_fs(desc.fs)
, m_fw(desc.fw)
, m_device(desc.device)
, m_imageImporterShaders(nullptr)
{
    m_codecs[(int)ImgFmt::Jpeg] = new JpegCodec;
    m_codecs[(int)ImgFmt::Png] = new PngCodec;
    m_codecs[(int)ImgFmt::Exr] = new ExrCodec;
    m_fw->addListener(this);
}

IImgCodec* TextureLoader::findCodec(const std::string& fileName)
{
    std::string ext;
    FileUtils::getFileExt(fileName, ext);    

    if (ext == "png")
    {
        return m_codecs[(int)ImgFmt::Png];
    }
    else if (ext == "jpg" || ext == "jpeg")
    {
        return m_codecs[(int)ImgFmt::Jpeg];
    }
    else if (ext == "exr")
    {
        return m_codecs[(int)ImgFmt::Exr];
    }

    return nullptr;
}

void TextureLoader::start()
{
    m_imageImporterShaders = new GpuImageImporterShaders(*m_device);
}

TextureLoader::~TextureLoader()
{
    m_fw->removeListener(this);
    for (auto* c : m_codecs)
        delete c;

    for (auto l : m_loadingStates)
    {
        cleanState(*l.second, true);
        freeLoadState(l.second);
    }
        
    for (auto s : m_freeLoaderStates)
        delete s;

    delete m_imageImporterShaders;
}

TextureLoadResult TextureLoader::loadTexture(const char* fileName)
{
    return loadTextureInternal(fileName);
}

TextureLoadResult TextureLoader::loadTextureInternal(const char* fileName, render::Texture existingTexture)
{
    if (m_imageImporterShaders == nullptr)
        return TextureLoadResult { TextureStatus::InvalidArguments, render::Texture(), "Texture loader not initialized" };

    std::string strName = fileName;
    IImgCodec* codec = findCodec(fileName);
    if (codec == nullptr)
    {
        std::stringstream ss;
        ss << "Texture extension not supported for file: " << fileName;
        return TextureLoadResult { TextureStatus::InvalidExtension, render::Texture(), ss.str() };
    }

    LoadingState& loadState = *allocateLoadState();
    loadState.codec = codec;

    GpuImageImporter* imageLoader = nullptr;
    if (loadState.imageImporter == nullptr)
    {
        imageLoader = new GpuImageImporter(*m_device, *m_imageImporterShaders);
        loadState.imageImporter = imageLoader;
    }
    else
    {
        imageLoader = (GpuImageImporter*)loadState.imageImporter;
    }

    if (!existingTexture.valid())
    {
        render::TextureDesc texDesc;
        texDesc.width = 1;
        texDesc.height = 1;
        texDesc.format = Format::RGBA_8_UNORM;
        texDesc.recreatable = true;
        loadState.texture = m_device->createTexture(texDesc);
    }
    else
    {
        loadState.texture = existingTexture;
    }

    imageLoader->texture() = loadState.texture;

    loadState.fileName = fileName;
    
    FileReadRequest request(fileName, [this, &loadState](FileReadResponse& response)
    {
        if (response.status == FileStatus::Reading)
        {
            loadState.fileBuffer.append((const u8*)response.buffer, response.size);
        }
        else if (response.status == FileStatus::Success)
        {
            ImgCodecResult codecResult = loadState.codec->decompress(loadState.fileBuffer.data(), loadState.fileBuffer.size(), *loadState.imageImporter);
            if (codecResult.success())
            {
                loadState.loadResult = TextureLoadResult { TextureStatus::Ok, render::Texture() };
                loadState.resolvedFileName = response.filePath;
                std::lock_guard lock(m_completeStatesMutex);
                m_completeStates.push(&loadState);
            }
            else
            {
                loadState.loadResult = TextureLoadResult { codecResult.status, render::Texture(), std::move(codecResult.message) };
                std::lock_guard lock(m_completeStatesMutex);
                m_completeStates.push(&loadState);
            }
        }
        else if (response.status == FileStatus::Fail)
        {
            std::stringstream ss;
            ss << "Could not find texture file: " << loadState.fileName;
            loadState.loadResult = TextureLoadResult { TextureStatus::FileNotFound, render::Texture(), ss.str() };

            std::lock_guard lock(m_completeStatesMutex);
            m_completeStates.push(&loadState);
        }
    });

    request.additionalRoots = m_additionalPaths;

    loadState.fileHandle = m_fs->read(request);

    {
        std::lock_guard lock(m_loadStateMutex);
        m_loadingStates[loadState.texture] = &loadState;
        m_fs->execute(loadState.fileHandle);
    }

    return TextureLoadResult { TextureStatus::Ok, loadState.texture };
}

void TextureLoader::cleanState(LoadingState& state, bool sync)
{
    if (sync)
        m_fs->wait(state.fileHandle);

    m_fs->closeHandle(state.fileHandle);
    state.clear();

    if (!sync)
        freeLoadState(&state);
}

void TextureLoader::processTextures()
{
    std::vector<LoadingState*> acquiredStates;
    {
        int loadBudget = 8;
        std::lock_guard lock(m_completeStatesMutex);
        while (!m_completeStates.empty() && loadBudget-- > 0)
        {
            auto* state = m_completeStates.front();
            acquiredStates.push_back(state);
            m_completeStates.pop();

            //This happens when a state was deleted.
            if (!state->texture.valid())
            {
                ++loadBudget;
                freeLoadState(state);
            }

            m_loadingStates.erase(state->texture);
        }
    }

    std::vector<render::CommandList*> lists;
    for (auto* state : acquiredStates)
    {
        if (state->loadResult.success())
        {
            auto* imageLoader = (GpuImageImporter*)state->imageImporter;
            auto texResult = m_device->recreateTexture(state->texture, imageLoader->textureDesc());
            CPY_ASSERT_MSG(texResult.success(), texResult.message.c_str()); //TODO: handle this failure
            if (texResult.success())
                lists.push_back(&imageLoader->cmdList());
        }
        else
        {
            CPY_ASSERT_MSG(false, state->loadResult.message.c_str()); //TODO handle this failure
        }
    }
        
    render::ScheduleStatus result = m_device->schedule(lists.data(), lists.size());
    if (!result.success())
    {
        CPY_ASSERT_FMT(false, "Error scheduling texture loading: %s", result.message.c_str());
    }

    for (auto* state : acquiredStates)
    {
        trackTexture(state->resolvedFileName.c_str(), state->texture);
        cleanState(*state, false);
    }
}

void TextureLoader::unloadTexture(render::Texture texture)
{
    {
        LoadingState* ls = nullptr;
        std::lock_guard lock(m_loadStateMutex);
        auto it = m_loadingStates.find(texture);
        if (it != m_loadingStates.end())
        {
            ls = it->second;
            m_loadingStates.erase(it);
        }
        if (ls != nullptr)
            cleanState(*ls, true);
    }

    {
        std::lock_guard lock(m_trackedFilesMutex);
        for (auto p : m_filesToTextures)
        {
            if (p.second == texture)
            {
                m_filesToTextures.erase(p.first);
                break;
            }
        }
    }

    m_device->release(texture);
}

TextureLoader::LoadingState* TextureLoader::allocateLoadState()
{
    std::lock_guard lock(m_loadStateMutex);
    if (m_freeLoaderStates.empty())
    {
        return new TextureLoader::LoadingState;
    }
    else
    {
        auto obj = m_freeLoaderStates.back();
        m_freeLoaderStates.pop_back();
        return obj;
    }
}

void TextureLoader::freeLoadState(TextureLoader::LoadingState* state)
{
    std::lock_guard lock(m_loadStateMutex);
    m_freeLoaderStates.push_back(state);
}

void TextureLoader::trackTexture(const std::string& resolvedFile, render::Texture texture)
{
    FileLookup lookup(resolvedFile);
    std::lock_guard lock(m_trackedFilesMutex);
    m_filesToTextures[lookup] = texture;
}

void TextureLoader::onFilesChanged(const std::set<std::string>& filesChanged)
{
    if (filesChanged.empty())
        return;

    std::lock_guard lock(m_trackedFilesMutex);
    for (auto fileChanged : filesChanged)
    {
        std::string resolvedFileName;
        FileUtils::getAbsolutePath(fileChanged, resolvedFileName);

        FileLookup lookup(resolvedFileName);
        auto it = m_filesToTextures.find(resolvedFileName);
        if (it == m_filesToTextures.end())
            continue;

        {
            std::lock_guard lock(m_loadStateMutex);
            if (m_loadingStates.find(it->second) != m_loadingStates.end())
                continue;
        }

        loadTextureInternal(resolvedFileName.c_str(), it->second);
    }
}

void TextureLoader::addPath(const char* path)
{
    m_additionalPaths.push_back(path);

    if (m_fw)
        m_fw->addDirectory(path);
}


ITextureLoader* ITextureLoader::create(const TextureLoaderDesc& desc)
{
    return new TextureLoader(desc);
}

}
