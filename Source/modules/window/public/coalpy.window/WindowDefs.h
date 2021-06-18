#pragma once

#include <coalpy.core/Os.h>
#include <functional>

namespace coalpy
{

class IWindow;

using OnRender = std::function<bool()>;

struct WindowDesc
{
    ModuleOsHandle osHandle;
    int width;
    int height;
};

struct WindowRunArgs
{
    OnRender onRender;
};

}

