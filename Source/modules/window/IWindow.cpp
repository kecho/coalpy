#include <coalpy.window/IWindow.h> 
#include <Config.h>

#if ENABLE_WIN32_WINDOW
#include "Win32Window.h"
#endif

namespace coalpy
{

IWindow* IWindow::create(const WindowDesc& desc)
{
#if ENABLE_WIN32_WINDOW
    return new Win32Window(desc);
#else
    //#error "No platform supported for IWindow"
    // TODO: implement Window for posix
    return nullptr;
#endif
}

}
