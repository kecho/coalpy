#pragma once

#include <coalpy.core/Os.h>
#include <functional>

namespace coalpy
{

class IWindow;

using OnRender = std::function<bool()>;

class IWindowListener
{
public:
    virtual ~IWindowListener() {}
    virtual void onClose (IWindow& window) = 0;
    virtual void onResize(int width, int height, IWindow& window) = 0;
};

struct WindowDesc
{
    ModuleOsHandle osHandle;
    int width;
    int height;
};

struct WindowRunArgs
{
    OnRender onRender;
    IWindowListener* listener = nullptr;
};

}

