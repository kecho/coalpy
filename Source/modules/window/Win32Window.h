#pragma once

#include <coalpy.window/IWindow.h>
#include <set>
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
    virtual void dimensions(int& w, int& h) const override;
    virtual ~Win32Window();

    struct HandleMessageRet
    {
        bool handled;
        unsigned long retCode;
    };

    void setListener(IWindowListener* listener) { m_listener = listener; }

    static std::set<Win32Window*> s_allWindows;

    HandleMessageRet handleMessage(
        unsigned message, unsigned int* wparam, unsigned long* lparam);

private:
    void createWindow();
    void destroyWindow();
    WindowDesc m_desc;
    Win32State& m_state;
    IWindowListener* m_listener = nullptr;
};

}

#endif
