#include "VulkanFormats.h"
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

namespace
{

const VkFormat g_formatTranslations[(int)Format::MAX_COUNT] =
{
    VK_FORMAT_R32G32B32A32_SFLOAT,//RGBA_32_FLOAT,
    VK_FORMAT_R32G32B32A32_UINT,//RGBA_32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,//RGBA_32_SINT,
    VK_FORMAT_R32G32B32A32_UINT,//RGBA_32_TYPELESS,
    VK_FORMAT_R32G32B32_SFLOAT,//RGB_32_FLOAT,
    VK_FORMAT_R32G32B32_UINT,//RGB_32_UINT,
    VK_FORMAT_R32G32B32_SINT,//RGB_32_SINT,
    VK_FORMAT_R32G32B32_UINT,//RGB_32_TYPELESS,
    VK_FORMAT_R32G32_SFLOAT,//RG_32_FLOAT,
    VK_FORMAT_R32G32_UINT,//RG_32_UINT,
    VK_FORMAT_R32G32_SINT,//RG_32_SINT,
    VK_FORMAT_R32G32_UINT,//RG_32_TYPELESS,
    VK_FORMAT_R16G16B16A16_SFLOAT,//RGBA_16_FLOAT,
    VK_FORMAT_R16G16B16A16_UINT,//RGBA_16_UINT,
    VK_FORMAT_R16G16B16A16_SINT,//RGBA_16_SINT,
    VK_FORMAT_R16G16B16A16_UNORM,//RGBA_16_UNORM,
    VK_FORMAT_R16G16B16A16_SNORM,//RGBA_16_SNORM,
    VK_FORMAT_R16G16B16A16_UINT,//RGBA_16_TYPELESS,
    VK_FORMAT_R8G8B8A8_UINT,//RGBA_8_UINT,
    VK_FORMAT_R8G8B8A8_SINT,//RGBA_8_SINT,
    VK_FORMAT_R8G8B8A8_UNORM,//RGBA_8_UNORM,
    VK_FORMAT_R8G8B8A8_SRGB,//RGBA_8_UNORM_SRGB,
    VK_FORMAT_R8G8B8A8_SNORM,//RGBA_8_SNORM,
    VK_FORMAT_R8G8B8A8_UINT,//RGBA_8_TYPELESS,
    VK_FORMAT_D32_SFLOAT,//D32_FLOAT,
    VK_FORMAT_R32_SFLOAT,//R32_FLOAT,
    VK_FORMAT_R32_UINT,//R32_UINT,
    VK_FORMAT_R32_SINT,//R32_SINT,
    VK_FORMAT_R32_UINT,//R32_TYPELESS,
    VK_FORMAT_D16_UNORM,//D16_UNORM,
    VK_FORMAT_R16_SFLOAT,//R16_FLOAT,
    VK_FORMAT_R16_UINT,//R16_UINT,
    VK_FORMAT_R16_SINT,//R16_SINT,
    VK_FORMAT_R16_UNORM,//R16_UNORM,
    VK_FORMAT_R16_SNORM,//R16_SNORM,
    VK_FORMAT_R16_UINT,//R16_TYPELESS,
    VK_FORMAT_R16G16_SFLOAT,//RG16_FLOAT,
    VK_FORMAT_R16G16_UINT,//RG16_UINT,
    VK_FORMAT_R16G16_SINT,//RG16_SINT,
    VK_FORMAT_R16G16_UNORM,//RG16_UNORM,
    VK_FORMAT_R16G16_SNORM,//RG16_SNORM,
    VK_FORMAT_R16G16_UINT,//RG16_TYPELESS,
    VK_FORMAT_R8_UNORM,//R8_UNORM,
    VK_FORMAT_R8_SINT,//R8_SINT,
    VK_FORMAT_R8_UINT,//R8_UINT,
    VK_FORMAT_R8_SNORM,//R8_SNORM,
    VK_FORMAT_R8_UNORM //R8_TYPELESS,
};

const int g_strides[(int)Format::MAX_COUNT] =
{
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

VkFormat getVkFormat(Format format)
{
    CPY_ERROR((int)format >= 0 && (int)format < (int)Format::MAX_COUNT);
    return g_formatTranslations[(int)format];
}

int getVkFormatStride(Format format)
{
    CPY_ERROR((int)format >= 0 && (int)format < (int)Format::MAX_COUNT);
    return g_strides[(int)format];
}

}
}
