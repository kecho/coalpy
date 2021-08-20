#pragma once

#include <coalpy.texture/ITextureLoader.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.render/CommandList.h>
#include "TextureLoader.h"

namespace coalpy
{

namespace render
{
    class IDevice;
}

enum class GpuImgImportShaders
{
    RgToRgba,
    RgbToRgba,
    Count
};

class GpuImageImporterShaders
{
public:
    GpuImageImporterShaders(const GpuImageImporterShaders& other);
    GpuImageImporterShaders(render::IDevice& device);

    ShaderHandle get(GpuImgImportShaders shaderId) const { return m_shaders[(int)shaderId]; }

private:
    ShaderHandle m_shaders[(int)GpuImgImportShaders::Count];
};

class GpuImageImporter : public IImgImporter
{
public:
    GpuImageImporter(render::IDevice& device, const GpuImageImporterShaders& shaders);
    virtual unsigned char* allocate(ImgColorFmt fmt, int width, int height, int bytes) override;
    virtual void clean() override;
    virtual ~GpuImageImporter();

    const render::TextureDesc& textureDesc() const { return m_texDesc; }
    render::Texture& texture() { return m_texture; }
    render::CommandList& cmdList() { return m_cmdList; }

private:
    render::IDevice& m_device;
    GpuImageImporterShaders m_shaders;
    render::Texture m_texture;
    render::TextureDesc m_texDesc;
    render::Buffer m_supportBuffer;
    render::InResourceTable m_supportInTable;
    render::OutResourceTable m_supportOutTable;
    render::CommandList m_cmdList;
};

}
