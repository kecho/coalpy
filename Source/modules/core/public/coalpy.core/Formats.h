#pragma once

namespace coalpy
{

//! Format list supported in pegasus.
//! \WARNING: some of these formats might not be available depending on the platform used.
enum class Format
{
    RGBA_32_FLOAT,
    RGBA_32_UINT,
    RGBA_32_SINT,
    RGBA_32_TYPELESS,
    RGB_32_FLOAT,
    RGB_32_UINT,
    RGB_32_SINT,
    RGB_32_TYPELESS,
    RG_32_FLOAT,
    RG_32_UINT,
    RG_32_SINT,
    RG_32_TYPELESS,
    RGBA_16_FLOAT,
    RGBA_16_UINT,
    RGBA_16_SINT,
    RGBA_16_UNORM,
    RGBA_16_SNORM,
    RGBA_16_TYPELESS,
    RGBA_8_UINT,
    RGBA_8_SINT,
    RGBA_8_UNORM,
    RGBA_8_UNORM_SRGB,
    RGBA_8_SNORM,
    RGBA_8_TYPELESS,
    D32_FLOAT,
    R32_FLOAT,
    R32_UINT,
    R32_SINT,
    R32_TYPELESS,
    D16_UNORM,
    R16_FLOAT,
    R16_UINT,
    R16_SINT,
    R16_UNORM,
    R16_SNORM,
    R16_TYPELESS,
    RG16_FLOAT,
    RG16_UINT,
    RG16_SINT,
    RG16_UNORM,
    RG16_SNORM,
    RG16_TYPELESS,
    R8_UNORM,
    R8_SINT,
    R8_UINT,
    R8_SNORM,
    R8_TYPELESS,
    MAX_COUNT
};


}
