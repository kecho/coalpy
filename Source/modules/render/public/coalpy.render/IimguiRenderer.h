#pragma once

#include <coalpy.core/RefCounted.h>

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
};

}
}
