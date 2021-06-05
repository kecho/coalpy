#pragma once

#include <coalpy.window/WindowDefs.h>

namespace coalpy
{

class IWindow
{
public:
    static IWindow* create(const WindowDesc& desc);
    virtual WindowOsHandle getHandle() const = 0;
    virtual ~IWindow() {}
};

}
