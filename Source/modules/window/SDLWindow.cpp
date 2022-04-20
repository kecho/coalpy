#include "SDLWindow.h"
#include <coalpy.core/Assert.h>
#include <iostream>

#if ENABLE_SDL_WINDOW 

#include <SDL2/SDL.h>

namespace coalpy
{

std::unordered_map<int, SDLWindow*> SDLWindow::s_allWindows;

SDLWindow::SDLWindow(const WindowDesc& desc)
: m_desc(desc)
{
    m_window = SDL_CreateWindow(
        desc.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        desc.width, desc.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    SDL_SetWindowData(m_window, "objptr", this);
    m_visible = true;
    s_allWindows.insert(std::make_pair<int,SDLWindow*>((int)SDL_GetWindowID(m_window), this));
}

SDLWindow::~SDLWindow()
{
    if (m_window != nullptr)
    {
        auto it = s_allWindows.find(SDL_GetWindowID(m_window));
        s_allWindows.erase(it);
        SDL_DestroyWindow(m_window);
    }
}

WindowOsHandle SDLWindow::getHandle() const
{
    return (WindowOsHandle)m_window;
}

void SDLWindow::open()
{
    if (m_window != nullptr)
        SDL_ShowWindow(m_window);
    m_visible = true;
}

bool SDLWindow::isClosed()
{
    return !m_visible;
}

void SDLWindow::close()
{
    if (m_window != nullptr)
        SDL_HideWindow(m_window);
    m_visible = false;
}

void SDLWindow::dimensions(int& w, int& h) const
{
    if (m_window == nullptr)
    {
        w = 0;
        h = 0;
        return;
    }

    SDL_GetWindowSize(m_window, &w, &h);
}

void SDLWindow::run(const WindowRunArgs& args)
{
    for (auto w : SDLWindow::s_allWindows)
        w.second->setListener(args.listener);

    bool finished = false;
    while (!finished)
    {
        SDL_Event event;
        int onMessage = SDL_PollEvent(&event);
        if (args.listener != nullptr)
        {
            if (event.type == SDL_WINDOWEVENT)
            {
                auto it = s_allWindows.find(event.window.windowID);
                CPY_ASSERT(it != s_allWindows.end());
                if (it != s_allWindows.end())
                {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        it->second->close();
                        args.listener->onClose(*it->second);
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        args.listener->onResize(event.window.data1, event.window.data2, *it->second);
                    }
                }
            }
        }

        if (args.onRender)
            finished = !args.onRender();
    }

    for (auto w : SDLWindow::s_allWindows)
        w.second->setListener(nullptr);
}

}
#endif
