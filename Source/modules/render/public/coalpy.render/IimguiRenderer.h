#pragma once

#include <coalpy.core/RefCounted.h>
#include <coalpy.render/Resources.h>
#include <imgui.h>

namespace coalpy
{

class IWindow;

namespace render
{

class IDevice;
class IDisplay;

struct IimguiRendererDesc
{
    IDevice* device = nullptr;
    IWindow* window = nullptr;
    IDisplay* display = nullptr;
};

class IimguiRenderer : public RefCounted
{
public:
    static IimguiRenderer* create(const IimguiRendererDesc& desc);

    virtual ~IimguiRenderer() {}
    virtual void newFrame() = 0;
    virtual void activate() = 0;
    virtual void render() = 0;
    virtual ImTextureID registerTexture(Texture texture) = 0;
    virtual void unregisterTexture(Texture texture) = 0;
    virtual bool isTextureRegistered(Texture texture) const = 0;
};

}
}
