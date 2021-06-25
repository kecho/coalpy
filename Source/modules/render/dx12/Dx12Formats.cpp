#include "Dx12Formats.h"
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

namespace
{

const DXGI_FORMAT g_formatTranslations[(int)Format::MAX_COUNT] =
{
       DXGI_FORMAT_R32G32B32A32_FLOAT,   // RGBA_32_FLOAT,
       DXGI_FORMAT_R32G32B32A32_UINT,    // RGBA_32_UINT,
       DXGI_FORMAT_R32G32B32A32_SINT,    // RGBA_32_SINT,
       DXGI_FORMAT_R32G32B32A32_TYPELESS,// RGBA_32_TYPELESS,
       DXGI_FORMAT_R32G32B32_FLOAT,      // RGB_32_FLOAT,
       DXGI_FORMAT_R32G32B32_UINT,       // RGB_32_UINT,
       DXGI_FORMAT_R32G32B32_SINT,       // RGB_32_SINT,
       DXGI_FORMAT_R32G32B32_TYPELESS,   // RGB_32_TYPELESS,
       DXGI_FORMAT_R32G32_FLOAT,         // RG_32_FLOAT,
       DXGI_FORMAT_R32G32_UINT,          // RG_32_UINT,
       DXGI_FORMAT_R32G32_SINT,          // RG_32_SINT,
       DXGI_FORMAT_R32G32_TYPELESS,      // RG_32_TYPELESS,
       DXGI_FORMAT_R16G16B16A16_FLOAT,   // RGBA_16_FLOAT,
       DXGI_FORMAT_R16G16B16A16_UINT,    // RGBA_16_UINT,
       DXGI_FORMAT_R16G16B16A16_SINT,    // RGBA_16_SINT,
       DXGI_FORMAT_R16G16B16A16_UNORM,   // RGBA_16_UNORM,
       DXGI_FORMAT_R16G16B16A16_SNORM,   // RGBA_16_SNORM,
       DXGI_FORMAT_R16G16B16A16_TYPELESS,// RGBA_16_TYPELESS,
       DXGI_FORMAT_R8G8B8A8_UINT,        // RGBA_8_UINT,
       DXGI_FORMAT_R8G8B8A8_SINT,        // RGBA_8_SINT,
       DXGI_FORMAT_R8G8B8A8_UNORM,       // RGBA_8_UNORM,
       DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  // RGBA_8_UNORM_SRGB,
       DXGI_FORMAT_R8G8B8A8_SNORM,       // RGBA_8_SNORM,
       DXGI_FORMAT_R8G8B8A8_TYPELESS,    // RGBA_8_TYPELESS,
       DXGI_FORMAT_D32_FLOAT,            // D32_FLOAT,
       DXGI_FORMAT_R32_FLOAT,            // R32_FLOAT,
       DXGI_FORMAT_R32_UINT,             // R32_UINT,
       DXGI_FORMAT_R32_SINT,             // R32_SINT,
       DXGI_FORMAT_R32_TYPELESS,         // R32_TYPELESS,
       DXGI_FORMAT_D16_UNORM,            // D16_FLOAT,
       DXGI_FORMAT_R16_FLOAT,            // R16_FLOAT,
       DXGI_FORMAT_R16_UINT,             // R16_UINT,
       DXGI_FORMAT_R16_SINT,             // R16_SINT,
       DXGI_FORMAT_R16_UNORM,            // R16_UNORM,
       DXGI_FORMAT_R16_SNORM,            // R16_SNORM,
       DXGI_FORMAT_R16_TYPELESS,         // R16_TYPELESS,
       DXGI_FORMAT_R16G16_FLOAT,         // RG16_FLOAT,
       DXGI_FORMAT_R16G16_UINT,          // RG16_UINT,
       DXGI_FORMAT_R16G16_SINT,          // RG16_SINT,
       DXGI_FORMAT_R16G16_UNORM,         // RG16_UNORM,
       DXGI_FORMAT_R16G16_SNORM,         // RG16_SNORM,
       DXGI_FORMAT_R16G16_TYPELESS,      // RG16_TYPELESS,
       DXGI_FORMAT_R8_UNORM,             // R8_UNORM
       DXGI_FORMAT_R8_SINT,              // R8_SINT
       DXGI_FORMAT_R8_UINT,              // R8_UINT
       DXGI_FORMAT_R8_SNORM,             // R8_SNORM
       DXGI_FORMAT_R8_TYPELESS           // R8_TYPELESS
};

const int g_strides[] = {
//b * c  // byte * components
  4 * 4 ,// RGBA_32_FLOAT,
  4 * 4 ,// RGBA_32_UINT,
  4 * 4 ,// RGBA_32_SINT,
  4 * 4 ,// RGBA_32_TYPELESS,
  4 * 3 ,// RGB_32_FLOAT,
  4 * 3 ,// RGB_32_UINT,
  4 * 3 ,// RGB_32_SINT,
  4 * 3 ,// RGB_32_TYPELESS,
  4 * 2 ,// RG_32_FLOAT,
  4 * 2 ,// RG_32_UINT,
  4 * 2 ,// RG_32_SINT,
  4 * 2 ,// RG_32_TYPELESS,
  2 * 4 ,// RGBA_16_FLOAT,
  2 * 4 ,// RGBA_16_UINT,
  2 * 4 ,// RGBA_16_SINT,
  2 * 4 ,// RGBA_16_UNORM,
  2 * 4 ,// RGBA_16_SNORM,
  2 * 4 ,// RGBA_16_TYPELESS,
  1 * 4 ,// RGBA_8_UINT,
  1 * 4 ,// RGBA_8_SINT,
  1 * 4 ,// RGBA_8_UNORM,
  1 * 4 ,// RGBA_8_UNORM_SRGB,
  1 * 4 ,// RGBA_8_SNORM,
  1 * 4 ,// RGBA_8_TYPELESS,
  4 * 1 ,// D32_FLOAT,
  4 * 1 ,// R32_FLOAT,
  4 * 1 ,// R32_UINT,
  4 * 1 ,// R32_SINT,
  4 * 1 ,// R32_TYPELESS,
  2 * 1 ,// D16_FLOAT,
  2 * 1 ,// R16_FLOAT,
  2 * 1 ,// R16_UINT,
  2 * 1 ,// R16_SINT,
  2 * 1 ,// R16_UNORM,
  2 * 1 ,// R16_SNORM,
  2 * 1 ,// R16_TYPELESS,
  2 * 2 ,// RG16_FLOAT,
  2 * 2 ,// RG16_UINT,
  2 * 2 ,// RG16_SINT,
  2 * 2 ,// RG16_UNORM,
  2 * 2 ,// RG16_SNORM,
  2 * 2 ,// RG16_TYPELESS,
  1 * 1 ,// R8_UNORM
  1 * 1 ,// R8_SINT
  1 * 1 ,// R8_UINT
  1 * 1 ,// R8_SNORM
  1 * 1  // R8_TYPELESS
};

}

DXGI_FORMAT getDxFormat(Format format)
{
    CPY_ERROR((int)format >= 0 && (int)format < (int)Format::MAX_COUNT);
    return g_formatTranslations[(int)format];
}

int getDxFormatStride(Format format)
{
    CPY_ERROR((int)format >= 0 && (int)format < (int)Format::MAX_COUNT);
    return g_strides[(int)format];
}


}
}
