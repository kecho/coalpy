#pragma once

namespace coalpy
{

typedef void* ModuleOsHandle;

struct WindowDesc
{
    ModuleOsHandle osHandle;
    int width;
    int height;
};

}

