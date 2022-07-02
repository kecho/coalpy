#pragma once

#include <coalpy.window/IWindow.h>
#include <coalpy.window/WindowInputState.h>
#include <set>
#include "Config.h"
#include <map>
#include <functional>

#if ENABLE_WIN32_WINDOW
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#endif

#if ENABLE_WIN32_WINDOW

namespace coalpy
{

struct Win32State;
using WindowHookFn = std::function<LRESULT(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

class Win32Window : public IWindow
{
public:
    explicit Win32Window(const WindowDesc& desc);
    virtual WindowOsHandle getHandle() const override;
    virtual void open() override;
    virtual bool isClosed() override;
    virtual bool shouldRender() override;
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

    HandleMessageRet handleInputMessage(
        unsigned message, unsigned int* wparam, unsigned long* lparam);

    int addHook(WindowHookFn hookFn);
    void removeHook(int hookId);

    static LRESULT CALLBACK win32WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    virtual const WindowInputState& inputState() const { return m_inputState; }

    static void run(const WindowRunArgs& runArgs);

private:
    void createWindow();
    void destroyWindow();
    WindowDesc m_desc;
    Win32State& m_state;
    IWindowListener* m_listener = nullptr;

    int m_nextHookId;
    std::map<int, WindowHookFn> m_hooks;

    void updateMousePosition();
    WindowInputState m_inputState;
};

}

#endif
