#pragma once

#include <coalpy.core/Formats.h>
#include <vulkan/vulkan.h>

namespace coalpy
{
namespace render
{

VkFormat getVkFormat(Format format);
int getVkFormatStride(Format format);

}
}
