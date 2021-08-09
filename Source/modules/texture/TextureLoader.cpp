#include "TextureLoader.h"
#include <coalpy.files/Utils.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/IShaderDb.h>
#include "JpegCodec.h"
#include "PngCodec.h"
#include <sstream>

namespace coalpy
{

static const char* s_srgb2SrgbaCode = R"(

Buffer<unorm float> g_inputBuffer : register(t0);
RWTexture2D<unorm float4> g_outputTexture : register(u0);

cbuffer Constants : register(b0)
{
    int4 texSizes;
}

[numthreads(8,8,1)]
void csMain(int2 dti : SV_DispatchThreadID)
{
    if (any(dti.xy >= texSizes.xy))
        return;

    int bufferOffset = 3 * (dti.y * texSizes.x + dti.y);
    g_outputTexture[dti.xy] = float4(g_inputBuffer[bufferOffset], g_inputBuffer[bufferOffset + 1], g_inputBuffer[bufferOffset + 2], 1.0); 
}
)";

class CmdListImageUploader : public IImgData
{
public:

    CmdListImageUploader(render::IDevice& device, ShaderHandle srgbToSrgbaShader)
    : m_device(device), m_srgbToSrgbaShader(srgbToSrgbaShader) {}

    render::Texture texture() { return m_texture; }
    render::CommandList& cmdList() { return m_cmdList; }

    virtual void clean() override
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

    virtual unsigned char* allocate(ImgColorFmt fmt, int width, int height, int bytes) override
    {
        render::TextureDesc texDesc;
        texDesc.memFlags = render::MemFlag_GpuRead;
        texDesc.type = render::TextureType::k2d;
        texDesc.width = width; 
        texDesc.height = height; 

        render::MemOffset offset = {};

        if (fmt == ImgColorFmt::sRgb)
        {
            if (!m_srgbToSrgbaShader.valid())
                return nullptr;

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
                texDesc.format = Format::RGBA_8_UNORM_SRGB;
                texDesc.memFlags = (render::MemFlags)((int)texDesc.memFlags | render::MemFlag_GpuWrite);
                m_texture = m_device.createTexture(texDesc);
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
                cmd.setShader(m_srgbToSrgbaShader);
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
                texDesc.format = Format::RGBA_8_UNORM_SRGB;
                break;          
            case ImgColorFmt::R:
                texDesc.format = Format::R8_UNORM;
                break;
            case ImgColorFmt::Rgba:
            default:
                texDesc.format = Format::RGBA_8_UNORM;
                break;
            }

            m_texture = m_device.createTexture(texDesc);
            CPY_ASSERT(m_texture.valid());
            if (!m_texture.valid())
                return nullptr;

            offset = m_cmdList.uploadInlineResource(m_texture, bytes);
            m_cmdList.finalize();
        }

        return m_cmdList.data() + offset;
    }

private:
    render::IDevice& m_device;
    render::Texture m_texture;
    render::Buffer m_supportBuffer;
    render::InResourceTable m_supportInTable;
    render::OutResourceTable m_supportOutTable;
    render::CommandList m_cmdList;
    ShaderHandle m_srgbToSrgbaShader;
};

TextureLoader::TextureLoader(const TextureLoaderDesc& desc)
: m_ts(desc.ts)
, m_fs(desc.fs)
, m_device(desc.device)
{
    m_codecs[(int)ImgFmt::Jpeg] = new JpegCodec;
    m_codecs[(int)ImgFmt::Png] = new PngCodec;
}

void TextureLoader::start()
{
    if (m_device->db() == nullptr)
        return;

    {
        IShaderDb& db = *m_device->db();
        ShaderInlineDesc desc = { ShaderType::Compute, "srgbToSrgba", "csMain", s_srgb2SrgbaCode };
        m_srgbToSrgba = db.requestCompile(desc);
        db.resolve(m_srgbToSrgba);
        CPY_ASSERT(db.isValid(m_srgbToSrgba));
    }
}

TextureLoader::~TextureLoader()
{
    for (auto* c : m_codecs)
        delete c;
}

TextureLoadResult TextureLoader::loadTexture(const char* fileName)
{
    std::string strName = fileName;
    IImgCodec* codec = findCodec(fileName);
    if (codec == nullptr)
    {
        std::stringstream ss;
        ss << "Texture extension not supported for file: " << fileName;
        return TextureLoadResult { TextureStatus::InvalidExtension, render::Texture(), ss.str() };
    }

    ByteBuffer bb;
    bool success = false;
    FileReadRequest request(fileName, [&bb, &success](FileReadResponse& response)
    {
        if (response.status == FileStatus::Reading)
        {
            bb.append((const u8*)response.buffer, response.size);
        }
        else if (response.status == FileStatus::Success)
        {
            success = true;
        }
    });

    AsyncFileHandle readHandle = m_fs->read(request);
    m_fs->execute(readHandle);
    m_fs->wait(readHandle);
    m_fs->closeHandle(readHandle);
    
    if (!success)
        return TextureLoadResult { TextureStatus::FileNotFound, render::Texture(), "File not found" };

    CmdListImageUploader image(*m_device, m_srgbToSrgba);

    ImgCodecResult result = codec->decompress(bb.data(), bb.size(), image);
    if (!result.success())
        return TextureLoadResult { result.status, render::Texture(), result.message };

    render::CommandList* listPtr = &image.cmdList();
    render::ScheduleStatus status = m_device->schedule(&listPtr, 1);
    image.clean();
    if (!status.success())
    {
        m_device->release(image.texture());
        std::stringstream ss;
        ss << "Gpu error while loading texture. Reason: " << status.message;
        return TextureLoadResult{ TextureStatus::InvalidArguments, render::Texture(), ss.str() };
    }
    return TextureLoadResult { TextureStatus::Ok, image.texture() };
}

void TextureLoader::processTextures()
{
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

    return nullptr;
}

ITextureLoader* ITextureLoader::create(const TextureLoaderDesc& desc)
{
    return new TextureLoader(desc);
}

}
