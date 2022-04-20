#include <coalpy.window/IWindow.h> 
#include <Config.h>

#if ENABLE_WIN32_WINDOW
#include "Win32Window.h"
#endif

#if ENABLE_SDL_WINDOW
#include "SDLWindow.h"
#endif

namespace coalpy
{

IWindow* IWindow::create(const WindowDesc& desc)
{
#if ENABLE_WIN32_WINDOW
    return new Win32Window(desc);
#elif ENABLE_SDL_WINDOW
    return new SDLWindow(desc);
#else
    //#error "No platform supported for IWindow"
    // TODO: implement Window for posix
    return nullptr;
#endif
}

void IWindow::run(const WindowRunArgs& args)
{
#if ENABLE_WIN32_WINDOW
    Win32Window::run(args);
#elif ENABLE_SDL_WINDOW
    SDLWindow::run(args);
#endif
}

}
