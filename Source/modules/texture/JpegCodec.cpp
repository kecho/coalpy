#include "JpegCodec.h"

#include <jpeglib.h>
#include <setjmp.h>
#include <sstream>

namespace coalpy
{
const char* jpegFormatToStr(J_COLOR_SPACE space)
{
    switch (space)
    {
	    case JCS_UNKNOWN:
            return "JCS_UNKNOWN";
	    case JCS_GRAYSCALE:
            return "JCS_GRAYSCALE";
	    case JCS_RGB:
            return "JCS_RGB";
	    case JCS_YCbCr:
            return "JCS_YCbCr";
	    case JCS_CMYK:
            return "JCS_CMYK";
	    case JCS_YCCK:
            return "JCS_YCCK";
	    case JCS_BG_RGB:
            return "JCS_BG_RGB";
	    case JCS_BG_YCC:
        default:
            return "JCS_BG_YCC";
    }
}

struct JpegCodecContext
{
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr errorMgr;
    jmp_buf jmpBuffer;	/* for return to caller on exit*/
    ImgCodecResult result;
};

METHODDEF(void) jpeglibErrorExitCb(j_common_ptr cinfo)
{
    JpegCodecContext& context = *reinterpret_cast<JpegCodecContext*>(cinfo);
    (*cinfo->err->output_message) (cinfo);
    longjmp(context.jmpBuffer, 1);
}

METHODDEF(void) jpeglibOutputMsgCb(j_common_ptr cinfo)
{
    JpegCodecContext& context = *reinterpret_cast<JpegCodecContext*>(cinfo);
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    context.result.status = TextureStatus::JpegCodecError;
    context.result.message = buffer;
}

ImgCodecResult JpegCodec::decompress(
    const unsigned char* buffer,
    size_t bufferSize,
    IImgData& outData)
{
    JpegCodecContext context;
    auto& cinfo = context.cinfo;
    cinfo.err = jpeg_std_error(&context.errorMgr);
    context.errorMgr.error_exit = jpeglibErrorExitCb;
    context.errorMgr.output_message = jpeglibOutputMsgCb;
    if (setjmp(context.jmpBuffer))
    {
        jpeg_destroy_decompress(&cinfo);
        return context.result;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, const_cast<unsigned char*>(buffer), bufferSize);

    ImgColorFmt fmt = ImgColorFmt::R;
    switch (cinfo.jpeg_color_space)
    {
    case JCS_RGB:
        switch (cinfo.output_components)
        {
        case 3:
            fmt = ImgColorFmt::sRgb;
        default:
            {
                std::stringstream ss;
                ss << "Only supporting RGB components for sRGB in jpeg decompressoin. Components used: " << cinfo.output_components;
                jpeg_destroy_decompress(&cinfo);
                context.result.status = TextureStatus::JpegFormatNotSupported;
                context.result.message = ss.str();
                return context.result;
            }
        }
        break;
    case JCS_GRAYSCALE:
        switch (cinfo.output_components)
        {
        case 1:
            fmt = ImgColorFmt::R;
        case 2:
            fmt = ImgColorFmt::Rg;
        case 3:
            fmt = ImgColorFmt::Rgb;
        case 4:
            fmt = ImgColorFmt::Rgba;
        }
        break;
    default:
        {
            jpeg_destroy_decompress(&cinfo);
            std::stringstream ss;
            ss << "Jpeg decompression format not supported: " << jpegFormatToStr(cinfo.jpeg_color_space);
            context.result.status = TextureStatus::JpegFormatNotSupported;
            context.result.message = ss.str();
            return context.result;
        }
    }

    jpeg_read_header(&cinfo, true);
    
    jpeg_start_decompress(&cinfo);

    int bytes = cinfo.output_components * cinfo.output_width * cinfo.output_height;
    unsigned char* imgData = outData.allocate(fmt, cinfo.output_width, cinfo.output_height, bytes);
    jpeg_read_scanlines(&cinfo, (JSAMPARRAY)imgData, cinfo.output_height);

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return ImgCodecResult { TextureStatus::Ok };
}


}
