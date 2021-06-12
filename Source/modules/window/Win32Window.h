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
    virtual WindowOsHandle getHandle() const override;
    virtual void open() override;
    virtual bool isClosed() override;
    virtual ~Win32Window();

    struct HandleMessageRet
    {
        bool handled;
        unsigned long retCode;
    };

    HandleMessageRet handleMessage(
        unsigned message, unsigned int* wparam, unsigned long* lparam);

private:
    void createWindow();
    void destroyWindow();
    WindowDesc m_desc;
    Win32State& m_state;
};

}

#endif
