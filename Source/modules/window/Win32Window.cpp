#pragma once

#include "Win32Window.h"
#include <coalpy.core/Assert.h>
#include <unordered_map>

//Uncomment this to print out some key states, to test input capture on a window.
#define TEST_WIN32_KEY_EVENTS 0
#if TEST_WIN32_KEY_EVENTS
#include <iostream>
#endif

namespace coalpy
{

#if ENABLE_WIN32_WINDOW

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
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    windowClass.lpfnWndProc = Win32Window::win32WinProc;
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
: m_desc(desc), m_state(*(new Win32State)), m_nextHookId(0)
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

int Win32Window::addHook(WindowHookFn hookFn)
{
    int hookId = m_nextHookId++;
    m_hooks[hookId] = hookFn;
    return hookId;
}

void Win32Window::removeHook(int hookId)
{
    auto it = m_hooks.find(hookId);
    CPY_ASSERT(it != m_hooks.end());
    if (it != m_hooks.end())
        m_hooks.erase(it);
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

static Keys translateKey(WPARAM p)
{
    switch (p)
    {
    case 'A': return Keys::A;
    case 'B': return Keys::B;
    case 'C': return Keys::C;
    case 'D': return Keys::D;
    case 'E': return Keys::E;
    case 'F': return Keys::F;
    case 'G': return Keys::G;
    case 'H': return Keys::H;
    case 'I': return Keys::I;
    case 'J': return Keys::J;
    case 'K': return Keys::K;
    case 'L': return Keys::L;
    case 'M': return Keys::M;
    case 'N': return Keys::N;
    case 'O': return Keys::O;
    case 'P': return Keys::P;
    case 'Q': return Keys::Q;
    case 'R': return Keys::R;
    case 'S': return Keys::S;
    case 'T': return Keys::T;
    case 'U': return Keys::U;
    case 'V': return Keys::V;
    case 'W': return Keys::W;
    case 'X': return Keys::X;
    case 'Y': return Keys::Y;
    case 'Z': return Keys::Z;
    case '0': case VK_NUMPAD0: return Keys::k0;
    case '1': case VK_NUMPAD1: return Keys::k1;
    case '2': case VK_NUMPAD2: return Keys::k2;
    case '3': case VK_NUMPAD3: return Keys::k3;
    case '4': case VK_NUMPAD4: return Keys::k4;
    case '5': case VK_NUMPAD5: return Keys::k5;
    case '6': case VK_NUMPAD6: return Keys::k6;
    case '7': case VK_NUMPAD7: return Keys::k7;
    case '8': case VK_NUMPAD8: return Keys::k8;
    case '9': case VK_NUMPAD9: return Keys::k9;
    case VK_F1: return Keys::F1;
    case VK_F2: return Keys::F2;
    case VK_F3: return Keys::F3;
    case VK_F4: return Keys::F4;
    case VK_F5: return Keys::F5;
    case VK_F6: return Keys::F6;
    case VK_F7: return Keys::F7;
    case VK_F8: return Keys::F8;
    case VK_F9: return Keys::F9;
    case VK_F10: return Keys::F10;
    case VK_F11: return Keys::F11;
    case VK_F12: return Keys::F12;
    case VK_SPACE: return Keys::Space;
    case VK_RETURN: return Keys::Enter;
    case '/':  return Keys::Slash;
    case '\\': return Keys::Backslash;
    case VK_BACK: return Keys::Backspace;
    case '[':  return Keys::LeftBrac;
    case ']':  return Keys::RightBrac;
    case ';':  return Keys::Semicolon;
    case VK_LSHIFT: return Keys::LeftShift;
    case VK_RSHIFT: return Keys::RightShift;
    case VK_LCONTROL: return Keys::LeftControl;
    case VK_RCONTROL: return Keys::RightControl;
    case VK_LMENU: return Keys::LeftAlt;
    case VK_RMENU: return Keys::RightAlt;
    case VK_LBUTTON: return Keys::MouseLeft;
    case VK_RBUTTON: return Keys::MouseRight;
    case VK_MBUTTON: return Keys::MouseCenter;
    default: return Keys::Unknown;
    }
}

Win32Window::HandleMessageRet Win32Window::handleInputMessage(unsigned message, unsigned int *wparam, unsigned long* lparam)
{
    HandleMessageRet ret{};

    //non sticky events:
    m_inputState.setKeyState(Keys::MouseLeftDouble, false);
    m_inputState.setKeyState(Keys::MouseRightDouble, false);

    switch (message)
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        {
            Keys k = translateKey((WPARAM)wparam);
            ret.handled = true;
            if (k != Keys::Unknown)
                m_inputState.setKeyState(k, true);
        }
    break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        {
            Keys k = translateKey((WPARAM)wparam);
            ret.handled = true;
            if (k != Keys::Unknown)
                m_inputState.setKeyState(k, false);
        }
    break;
    case WM_LBUTTONDOWN:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseLeft, true);
        break;
    case WM_LBUTTONUP:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseLeft, false);
        break;
    case WM_RBUTTONDOWN:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseRight, true);
        break;
    case WM_RBUTTONUP:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseRight, false);
        break;
    case WM_MBUTTONDOWN:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseCenter, true);
        break;
    case WM_MBUTTONUP:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseCenter, false);
        break;
    case WM_LBUTTONDBLCLK:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseLeftDouble, true);
        break;
    case WM_RBUTTONDBLCLK:
        ret.handled = true;
        m_inputState.setKeyState(Keys::MouseRightDouble, true);
        break;
    }
    return ret;
}

Win32Window::HandleMessageRet Win32Window::handleMessage(
    unsigned message,
    unsigned int* wparam,
    unsigned long* lparam)
{
    HandleMessageRet ret = {};

    //reset non sticky keys
    auto inputRet = handleInputMessage(message, wparam, lparam);

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

    if (inputRet.handled)
        ret.handled = true;
    return ret;
}

LRESULT CALLBACK Win32Window::win32WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
        for (auto it : self->m_hooks)
        {
            LRESULT res = it.second(hwnd, message, wParam, lParam);
            if (res != 0)
                return res;
        }

        auto ret = self->handleMessage(message, (unsigned int*)wParam, (unsigned long*)lParam);

//Testing
#if TEST_WIN32_KEY_EVENTS
        if (ret.handled)
        {
            if (self->m_inputState.keyState(Keys::G))
                std::cout << "Key G" << std::endl;
            if (self->m_inputState.keyState(Keys::MouseLeft))
                std::cout << "Key MouseLeft" << std::endl;
            if (self->m_inputState.keyState(Keys::MouseRight))
                std::cout << "Key MouseRight" << std::endl;
            if (self->m_inputState.keyState(Keys::MouseCenter))
                std::cout << "Key MouseCenter" << std::endl;
            if (self->m_inputState.keyState(Keys::MouseRightDouble))
                std::cout << "Key MouseRightDouble" << std::endl;
            if (self->m_inputState.keyState(Keys::MouseLeftDouble))
                std::cout << "Key MouseLeftDouble" << std::endl;
        }
#endif
    
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

