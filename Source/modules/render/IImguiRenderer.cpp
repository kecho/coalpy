#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.render/IDevice.h>
#include <Config.h>
#if ENABLE_DX12
#include "dx12/Dx12ImguiRenderer.h"
#endif
#if ENABLE_VULKAN
#include "vulkan/VulkanImguiRenderer.h"
#endif

namespace coalpy
{

namespace render
{
IimguiRenderer* IimguiRenderer::create(const IimguiRendererDesc& desc)
{
    if (desc.device == nullptr || desc.window == nullptr || desc.display == nullptr)
    {
        CPY_ERROR_MSG(false, "Invalid arguments for IimguiRenderer.");
        return nullptr;
    }

    DevicePlat platform = desc.device->config().platform;
#if ENABLE_DX12
    if (platform == DevicePlat::Dx12)
        return new Dx12imguiRenderer(desc);
#endif

#if ENABLE_VULKAN
    if (platform == DevicePlat::Vulkan)
        return new VulkanImguiRenderer(desc);
#endif

    return nullptr;
}

}
}
