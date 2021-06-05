#pragma once

#include <coalpy.window/IWindow.h>
#include "Config.h"

#if ENABLE_WIN32_WINDOW

namespace coalpy
{

struct Win32State;
class Win32Window : public IWindow
{
public:
    explicit Win32Window(const WindowDesc& desc);
    virtual ModuleOsHandle getHandle() const override;
    virtual ~Win32Window();

private:
    void createWindow();
    void destroyWindow();
    WindowDesc m_desc;
    Win32State& m_state;
};

}

#endif
