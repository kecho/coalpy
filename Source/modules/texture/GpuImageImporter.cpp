#include "GpuImageImporter.h"
#include <coalpy.render/IDevice.h>
#include <coalpy.render/IShaderDb.h>

namespace coalpy
{

static const char* s_rgbToRgbaSourceCode = R"(

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

    int bufferOffset = 3 * (dti.y * texSizes.x + dti.x);
    g_outputTexture[dti.xy] = float4(g_inputBuffer[bufferOffset], g_inputBuffer[bufferOffset + 1], g_inputBuffer[bufferOffset + 2], 1.0); 
}

)";

GpuImageImporterShaders::GpuImageImporterShaders(render::IDevice& device)
{
    CPY_ASSERT_MSG(device.db() != nullptr, "Device must not be null for GpuImageImporterShaders, image importing will fail.");
    if (device.db() == nullptr)
        return;

    {
        IShaderDb& db = *device.db();
        ShaderInlineDesc desc = { ShaderType::Compute, "rgbToRgba", "csMain", s_rgbToRgbaSourceCode };
        m_shaders[(int)GpuImgImportShaders::RgbToRgba] = db.requestCompile(desc);

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

unsigned char* GpuImageImporter::allocate(ImgColorFmt fmt, int width, int height, int bytes)
{
    render::TextureDesc& texDesc = m_texDesc;
    //texDesc.memFlags = render::MemFlag_GpuRead;
    texDesc.type = render::TextureType::k2d;
    texDesc.width = width; 
    texDesc.height = height;
    texDesc.format = Format::RGBA_8_UNORM;
    texDesc.recreatable = true;
    
    render::MemOffset offset = {};
    
    if (fmt == ImgColorFmt::sRgb || fmt == ImgColorFmt::Rgba32 || fmt == ImgColorFmt::Rgb32)
    {
        {
            render::BufferDesc bufferDesc;
            bufferDesc.format = Format::R8_UNORM;
            bufferDesc.elementCount = width * height * 3;
            bufferDesc.memFlags = render::MemFlag_GpuRead;
            CPY_ASSERT(bytes == bufferDesc.elementCount);
            if (bytes != bufferDesc.elementCount)
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
    
        int constData[4] = { width, height, 0, 0 };
        {
            render::ComputeCommand cmd;
            cmd.setShader(m_shaders.get(GpuImgImportShaders::RgbToRgba));
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
        switch (fmt)
        {
        case ImgColorFmt::sRgba:
            texDesc.format = Format::RGBA_8_UNORM;
            break;          
        case ImgColorFmt::R:
            texDesc.format = Format::R8_UNORM;
            break;
        case ImgColorFmt::R32:
            texDesc.format = Format::R32_FLOAT;
            break;
        case ImgColorFmt::Rgba:
        default:
            texDesc.format = Format::RGBA_8_UNORM;
            break;
        }
    
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
