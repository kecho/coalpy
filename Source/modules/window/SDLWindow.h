#pragma once

#include <coalpy.window/IWindow.h>
#include <coalpy.window/WindowInputState.h>
#include <set>
#include "Config.h"
#include <unordered_map>
#include <functional>

#if ENABLE_SDL_WINDOW

struct SDL_Window;
union SDL_Event;

namespace coalpy
{

using SDLEventCallback = std::function<bool(const SDL_Event*)>;

class SDLWindow : public IWindow
{
public:
    explicit SDLWindow(const WindowDesc& desc);
    virtual ~SDLWindow();
    virtual WindowOsHandle getHandle() const override;
    virtual void open() override;
    virtual bool isClosed() override;
    virtual bool shouldRender() { return !isClosed() && m_desc.width > 0 && m_desc.height > 0; }
    virtual void dimensions(int& w, int& h) const override;
    virtual const WindowInputState& inputState() const { return m_inputState; }
    void setListener(IWindowListener* listener) { m_listener = listener; }

    void close();
    static void run(const WindowRunArgs& runArgs);

    static std::unordered_map<int, SDLWindow*> s_allWindows;

    static void SetEventCallback(SDLEventCallback cb) { s_callback = cb; }
private:
    static SDLEventCallback s_callback;
    WindowDesc m_desc;
    int m_windowID = {};
    WindowInputState m_inputState = {};
    IWindowListener* m_listener = nullptr;
    SDL_Window* m_window = {};
    bool m_visible = true;
};

}

#endif
