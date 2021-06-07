#pragma once

#include <functional>

namespace coalpy
{

typedef void* ModuleOsHandle;
typedef void* WindowOsHandle;

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

