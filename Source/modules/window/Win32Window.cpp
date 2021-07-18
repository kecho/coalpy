#pragma once

#include "Win32Window.h"
#include <coalpy.core/Assert.h>
#include <unordered_map>

#if ENABLE_WIN32_WINDOW
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#endif

namespace coalpy
{

#if ENABLE_WIN32_WINDOW

LRESULT CALLBACK win32WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace
{

const char* g_windowClassName = "coalpy.window";
const char* g_windowName = "coalpy.window";
std::unordered_map<ModuleOsHandle, int> g_windowClassRefCounts;

void registerWindowClass(ModuleOsHandle handle)
{
    if (!handle)
        return;

    auto it = g_windowClassRefCounts.find(handle);
    if (it == g_windowClassRefCounts.end())
    {
        g_windowClassRefCounts[handle] = 0;
        it = g_windowClassRefCounts.find(handle);
    }

    ++it->second;
    if (it->second > 1)
        return;

    WNDCLASSEX windowClass;

    // Set up our window class
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = win32WinProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = (HINSTANCE)handle;
    windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = g_windowClassName;
    windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    WND_OK(RegisterClassEx(&windowClass));
}

void unregisterWindowClass(ModuleOsHandle handle)
{
    if (!handle)
        return;

    auto it = g_windowClassRefCounts.find(handle);
    if (it == g_windowClassRefCounts.end())
        return;

    --it->second;
    if (it->second == 0)
    {
        WND_OK(UnregisterClass(g_windowClassName, (HINSTANCE)handle));
        g_windowClassRefCounts.erase(it);
    }
}

}

struct Win32State
{
    HWND windowHandle;
};

std::set<Win32Window*> Win32Window::s_allWindows;

Win32Window::Win32Window(const WindowDesc& desc)
: m_desc(desc), m_state(*(new Win32State))
{
    CPY_ASSERT_MSG(desc.osHandle != nullptr, "No os handle set, window wont be able to register its class.");
    registerWindowClass(desc.osHandle);
    createWindow();
    Win32Window::s_allWindows.insert(this);
}

Win32Window::~Win32Window()
{
    Win32Window::s_allWindows.erase(this);
    destroyWindow();
    unregisterWindowClass(m_desc.osHandle);
    delete &m_state;
}

void Win32Window::createWindow()
{
    // Compute window style
    DWORD windowStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE;

    // Account for the title bar and the borders
    RECT rect;
    rect.left = 0;
    rect.right = m_desc.width;
    rect.top = 0;
    rect.bottom = m_desc.height;
    AdjustWindowRectEx(&rect, windowStyle, FALSE, /*windowStyleEx*/0);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    m_state.windowHandle = CreateWindowEx(0,
                                  g_windowClassName,
                                  m_desc.title.c_str(),
                                  windowStyle,
                                  100, 100,
                                  windowWidth, windowHeight,
                                  NULL,
                                  NULL,
                                  (HINSTANCE) m_desc.osHandle,
                                  (LPVOID) this);
    if (m_state.windowHandle == NULL)
    {
        DWORD err = GetLastError();
        CPY_ASSERT_FMT(false, "Unable to create window! %d(%X)", err, err);
    }
}

void Win32Window::destroyWindow()
{
    if (m_state.windowHandle != nullptr)
        WND_OK(DestroyWindow(m_state.windowHandle));
}

WindowOsHandle Win32Window::getHandle() const
{
    return (WindowOsHandle)m_state.windowHandle;
}

bool Win32Window::isClosed()
{
    return (m_state.windowHandle == nullptr);
}

void Win32Window::open()
{
    if (isClosed())
        open();
}

void Win32Window::dimensions(int& w, int& h) const
{
    w = m_desc.width;
    h = m_desc.height;
}

Win32Window::HandleMessageRet Win32Window::handleMessage(
    unsigned message,
    unsigned int* wparam,
    unsigned long* lparam)
{
    HandleMessageRet ret = {};
    switch (message)
    {
    case WM_DESTROY:
        ret.handled = true;
        m_state.windowHandle = nullptr;
        if (m_listener)
        {
            m_listener->onClose(*this);
        }
        break;
    case WM_SIZE:
        if (m_listener)
        {
            int w = (int)LOWORD(lparam);
            int h = (int)HIWORD(lparam);
            m_desc.width = w;
            m_desc.height = h;
            m_listener->onResize(w, h, *this);
        }
        ret.handled = true;
        break;
    }
    return ret;
}

LRESULT CALLBACK win32WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE)
    {
        auto self = static_cast<Win32Window*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        if (self)
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    }

    auto self = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (self)
    {
        auto ret = self->handleMessage(message, (unsigned int*)wParam, (unsigned long*)lParam);
        if (ret.handled)
            return (LRESULT)ret.retCode;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

#endif

IWindow* IWindow::create(const WindowDesc& desc)
{
#if ENABLE_WIN32_WINDOW
    return new Win32Window(desc);
#else
    #error "No platform supported for IWindow"
    return nullptr;
#endif
}

void IWindow::run(const WindowRunArgs& args)
{
#if ENABLE_WIN32_WINDOW
    for (auto w : Win32Window::s_allWindows)
        w->setListener(args.listener);

    bool finished = false;
    MSG currMsg;
    while (!finished)
    {
        bool onMessage = PeekMessage(&currMsg, NULL, NULL, NULL, PM_REMOVE);
        //run all windows here
        if (args.onRender)
            finished = !args.onRender();

        if (onMessage)
        {
            TranslateMessage(&currMsg);
            DispatchMessage(&currMsg);
        }
    }

    for (auto w : Win32Window::s_allWindows)
        w->setListener(nullptr);
#endif
}


}

