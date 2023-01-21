#include "GpuImageImporter.h"
#include <coalpy.render/IDevice.h>
#include <coalpy.render/IShaderDb.h>

namespace coalpy
{

static const char* s_rgbToRgbaSourceCode = R"(

#ifndef CHANNELS_USED
#define CHANNELS_USED 3
#endif

Buffer<unorm float> g_inputBuffer : register(t0);
RWTexture2D<float4> g_outputTexture : register(u0);

cbuffer Constants : register(b0)
{
    int4 texSizes;
}

[numthreads(8,8,1)]
void csMain(int2 dti : SV_DispatchThreadID)
{
    if (any(dti.xy >= texSizes.xy))
        return;

    int bufferOffset = CHANNELS_USED * (dti.y * texSizes.x + dti.x);
    #if CHANNELS_USED == 2
        g_outputTexture[dti.xy] = float4(g_inputBuffer[bufferOffset], g_inputBuffer[bufferOffset + 1], 1.0, 1.0); 
    #elif CHANNELS_USED == 3
        g_outputTexture[dti.xy] = float4(g_inputBuffer[bufferOffset], g_inputBuffer[bufferOffset + 1], g_inputBuffer[bufferOffset + 2], 1.0); 
    #elif CHANNELS_USED == 4
        g_outputTexture[dti.xy] = float4(g_inputBuffer[bufferOffset], g_inputBuffer[bufferOffset + 1], g_inputBuffer[bufferOffset + 2], g_inputBuffer[bufferOffset + 3]); 
    #endif
}

)";

GpuImageImporterShaders::GpuImageImporterShaders(render::IDevice& device)
{
    CPY_ASSERT_MSG(device.db() != nullptr, "Device must not be null for GpuImageImporterShaders, image importing will fail.");
    if (device.db() == nullptr)
        return;

    {
        IShaderDb& db = *device.db();
        {
            ShaderInlineDesc desc = { ShaderType::Compute, "rgToRgba", "csMain", s_rgbToRgbaSourceCode, { "CHANNELS_USED=2" } };
            m_shaders[(int)GpuImgImportShaders::RgToRgba] = db.requestCompile(desc);
        }
        {
            ShaderInlineDesc desc = { ShaderType::Compute, "rgbToRgba", "csMain", s_rgbToRgbaSourceCode, { "CHANNELS_USED=3" } };
            m_shaders[(int)GpuImgImportShaders::RgbToRgba] = db.requestCompile(desc);
        }

        for (auto s : m_shaders)
        {
            db.resolve(s);
            CPY_ASSERT(db.isValid(s));
        }
    }
}

GpuImageImporterShaders::GpuImageImporterShaders(const GpuImageImporterShaders& other)
{
    *this = other;
}

GpuImageImporter::GpuImageImporter(render::IDevice& device, const GpuImageImporterShaders& shaders)
: m_device(device), m_shaders(shaders)
{
}

struct SrcFormatInfo
{
    bool isDirectCopy = false;
    int componentCount = 1;
    size_t componentByteSize = 1;
    Format outputTextureFormat = Format::R8_UNORM;
    GpuImgImportShaders importShader = GpuImgImportShaders::RgbToRgba;
};

static void GetSrcFormatInfo(ImgColorFmt fmt, SrcFormatInfo& info)
{
    switch (fmt)
    {
    case ImgColorFmt::R:
        info = { true, 1, sizeof(char), Format::R8_UNORM };
        break;
    case ImgColorFmt::Rg:
        info = { false, 2, sizeof(char), Format::RGBA_8_UNORM, GpuImgImportShaders::RgToRgba };
        break;
    case ImgColorFmt::Rgb:
        info = { false, 3, sizeof(char), Format::RGBA_8_UNORM, GpuImgImportShaders::RgbToRgba };
        break;
    case ImgColorFmt::Rgba:
        info = { true, 4, sizeof(char), Format::RGBA_8_UNORM };
        break;
    case ImgColorFmt::sRgb:
        info = { false, 3, sizeof(char), Format::RGBA_8_UNORM_SRGB, GpuImgImportShaders::RgbToRgba };
        break;
    case ImgColorFmt::sRgba:
        info = { true, 4, sizeof(char), Format::RGBA_8_UNORM_SRGB };
        break;
    case ImgColorFmt::R32:
        info = { true, 1, sizeof(float), Format::R32_FLOAT };
        break;
    case ImgColorFmt::Rg32:
        info = { true, 2, sizeof(float), Format::RG_32_FLOAT };
        break;
    case ImgColorFmt::Rgb32:
        info = { true, 3, sizeof(float), Format::RGB_32_FLOAT };
        break;
    default:
    case ImgColorFmt::Rgba32:
        info = { true, 4, sizeof(float), Format::RGBA_32_FLOAT };
        break;
    }
}

unsigned char* GpuImageImporter::allocate(ImgColorFmt fmt, int width, int height, int bytes)
{
    render::TextureDesc& texDesc = m_texDesc;
    texDesc.type = render::TextureType::k2d;
    texDesc.width = width; 
    texDesc.height = height;
    texDesc.recreatable = true;
    
    render::MemOffset offset = {};

    SrcFormatInfo formatInfo;
    GetSrcFormatInfo(fmt, formatInfo);

    texDesc.format = formatInfo.outputTextureFormat;

    if (!formatInfo.isDirectCopy)
    {
        {
            render::BufferDesc bufferDesc;
            bufferDesc.format = formatInfo.componentByteSize == 1 ? Format::R8_UNORM : Format::R32_FLOAT;
            bufferDesc.elementCount = width * height * formatInfo.componentCount;
            bufferDesc.memFlags = render::MemFlag_GpuRead;
            CPY_ASSERT(bytes == bufferDesc.elementCount * formatInfo.componentByteSize);
            if (bytes != bufferDesc.elementCount * formatInfo.componentByteSize)
                return nullptr;
            m_supportBuffer = m_device.createBuffer(bufferDesc);
        }
    
        {
            render::ResourceTableDesc tableConfig;
            tableConfig.resources = &m_supportBuffer;
            tableConfig.resourcesCount = 1;
            m_supportInTable = m_device.createInResourceTable(tableConfig);
        }
    
        {
            render::ResourceTableDesc tableConfig;
            tableConfig.resources = &m_texture;
            tableConfig.resourcesCount = 1;
            m_supportOutTable = m_device.createOutResourceTable(tableConfig);
        }
    
        {
            offset = m_cmdList.uploadInlineResource(m_supportBuffer, bytes);
        }
    
        int constData[4] = { width, height, width * height, 0 };
        {
            render::ComputeCommand cmd;
            cmd.setShader(m_shaders.get(formatInfo.importShader));
            cmd.setInlineConstant((const char*)&constData, sizeof(constData));
            cmd.setInResources(&m_supportInTable, 1);
            cmd.setOutResources(&m_supportOutTable, 1);
            cmd.setDispatch("srgb to srgba copy", (width + 7)/8, (height + 7)/8, 1);
            m_cmdList.writeCommand(cmd);
        }
    
        m_cmdList.finalize();
    }
    else
    {
        texDesc.memFlags = render::MemFlag_GpuRead;
        CPY_ASSERT(m_texture.valid());
        if (!m_texture.valid())
            return nullptr;
    
        offset = m_cmdList.uploadInlineResource(m_texture, bytes);
        m_cmdList.finalize();
    }

    return m_cmdList.data() + offset;
}

void GpuImageImporter::clean()
{
    m_cmdList.reset();
    if (m_supportBuffer.valid())
    {
        m_device.release(m_supportBuffer);
        m_supportBuffer = render::Buffer();
    }
    
    if (m_supportInTable.valid())
    {
        m_device.release(m_supportInTable);
        m_supportInTable = render::InResourceTable();
    }
    
    if (m_supportOutTable.valid())
    {
        m_device.release(m_supportOutTable);
        m_supportOutTable = render::OutResourceTable();
    }
}

GpuImageImporter::~GpuImageImporter() 
{
    clean();
}

}
