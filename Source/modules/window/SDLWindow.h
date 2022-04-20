#pragma once

#include <coalpy.window/IWindow.h>
#include <coalpy.window/WindowInputState.h>
#include <set>
#include "Config.h"
#include <unordered_map>
#include <functional>

#if ENABLE_SDL_WINDOW

struct SDL_Window;

namespace coalpy
{

class SDLWindow : public IWindow
{
public:
    explicit SDLWindow(const WindowDesc& desc);
    virtual ~SDLWindow();
    virtual WindowOsHandle getHandle() const override;
    virtual void open() override;
    virtual bool isClosed() override;
    virtual void dimensions(int& w, int& h) const override;
    virtual const WindowInputState& inputState() const { return m_inputState; }
    void setListener(IWindowListener* listener) { m_listener = listener; }

    void close();
    static void run(const WindowRunArgs& runArgs);

    static std::unordered_map<int, SDLWindow*> s_allWindows;
private:
    WindowDesc m_desc;
    int m_windowID = {};
    WindowInputState m_inputState = {};
    IWindowListener* m_listener = nullptr;
    SDL_Window* m_window = {};
    bool m_visible = true;
};

}

#endif
