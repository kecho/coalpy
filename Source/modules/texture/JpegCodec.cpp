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
    IImgImporter& outData)
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
    jpeg_read_header(&cinfo, true);
    cinfo.out_color_space = JCS_RGB;    

    jpeg_start_decompress(&cinfo);

    ImgColorFmt fmt = ImgColorFmt::sRgb;
    switch (cinfo.out_color_components)
    {    
    case 3:
        fmt = ImgColorFmt::sRgb; break;
    case 4:
        fmt = ImgColorFmt::sRgba; break;
    default:
        {
            jpeg_destroy_decompress(&cinfo);
            std::stringstream ss;
            ss << "Jpeg decompression format not supported: " << jpegFormatToStr(cinfo.jpeg_color_space) << " components: " << cinfo.out_color_components;
            context.result.status = TextureStatus::JpegFormatNotSupported;
            context.result.message = ss.str();
            return context.result;
        }
    }

    int stride = cinfo.output_components * cinfo.output_width;
    int bytes = stride * cinfo.output_height;
    unsigned char* imgData = outData.allocate(fmt, cinfo.output_width, cinfo.output_height, bytes);
    if (!imgData)
    {
        jpeg_destroy_decompress(&cinfo);
        std::stringstream ss;
        ss << "Error allocating memory for image.";
        return ImgCodecResult { TextureStatus::CorruptedFile, ss.str() };
    }

    for (int i = 0; i < cinfo.output_height; ++i)
    {
        unsigned char* scanLine = imgData + stride * i;
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&scanLine, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return ImgCodecResult { TextureStatus::Ok };
}


}
