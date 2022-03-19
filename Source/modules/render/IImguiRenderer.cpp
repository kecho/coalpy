#include <coalpy.render/IimguiRenderer.h>
#include <Config.h>
#if ENABLE_DX12
#include "dx12/Dx12ImguiRenderer.h"
#endif

namespace coalpy
{

namespace render
{

IimguiRenderer* IimguiRenderer::create(const IimguiRendererDesc& desc)
{
#if ENABLE_DX12
    if (desc.device == nullptr || desc.window == nullptr || desc.display == nullptr)
    {
        CPY_ERROR_MSG(false, "Invalid arguments for IimguiRenderer.");
        return nullptr;
    }

    return new Dx12imguiRenderer(desc);
#else
    return nullptr;
#endif
}

}
}
