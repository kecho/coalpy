#pragma once

#include <coalpy.core/Os.h>
#include <functional>
#include <string>

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
    std::string title;
    int width;
    int height;
};

struct WindowRunArgs
{
    OnRender onRender;
    IWindowListener* listener = nullptr;
};

}

