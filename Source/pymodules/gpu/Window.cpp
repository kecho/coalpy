#include "Window.h"
#include <iostream>
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.render/IDevice.h>
#include "CoalpyTypeObject.h"

extern coalpy::ModuleOsHandle g_ModuleInstance;

namespace coalpy
{
namespace gpu
{

void Window::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Window";
    t.tp_basicsize = sizeof(Window);
    t.tp_doc   = "Class that represnts a window.\n"
                 "Constructor Arguments:\n"
                 "title  : String title for the window.\n"
                 "width  : initial width of window and swap chain texture.\n"
                 "height : initial height of window and swap chain texture.\n"
                 "on_render : Rendering function. The function has 1 argument of type RenderArgs and no return. See RenderArgs for more info.\n";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Window::init;
    t.tp_dealloc = Window::destroy;
}

int Window::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;


    auto& window = *(Window*)self;
    const char* windowTitle = "coalpy.gpu.window";
    WindowDesc desc;
    desc.osHandle = g_ModuleInstance;
    desc.width = 400;
    desc.height = 400;
    window.onRenderCallback = nullptr;

    static char* keywords[] = { "title", "width", "height", "on_render", nullptr };
    if (!PyArg_ParseTupleAndKeywords(
            vargs, kwds, "|siiO:Window", keywords,
            &windowTitle, &desc.width, &desc.height, &window.onRenderCallback))
    {
        return -1;
    }

    desc.title = windowTitle;

    Py_XINCREF(window.onRenderCallback);
    if (!PyCallable_Check(window.onRenderCallback))
    {
        Py_XDECREF(window.onRenderCallback);
        return -1;
    }

    window.object = IWindow::create(desc);
    window.object->setUserData(self);
    {
        moduleState.registerWindow(&window);
    }

    //create display / swap chain object for this window
    {
        render::IDevice& device = moduleState.device();
        render::DisplayConfig displayConfig;
        displayConfig.handle = window.object->getHandle();
        displayConfig.width = desc.width;
        displayConfig.height = desc.height;
        window.display = device.createDisplay(displayConfig);
    }
    
    return 0;
}

void Window::close(PyObject* self)
{
    auto& window = *(Window*)self;
    if (!window.object)
        return;

    Py_XDECREF(window.onRenderCallback);
    window.onRenderCallback = nullptr;
    delete window.object;
    window.object = nullptr;

    {
        ModuleState& moduleState = parentModule(self);
        moduleState.unregisterWindow(&window);
    }
}

void Window::destroy(PyObject* self)
{
    Window::close(self);
    Py_TYPE(self)->tp_free(self);
}

class ModuleWindowListener : public IWindowListener
{
public:
    ModuleWindowListener(ModuleState& moduleState)
    : m_moduleState(moduleState)
    {
    }

    virtual void onClose(IWindow& window) override
    {
        Window* pyWindow = (Window*)window.userData();
        if (!pyWindow || !pyWindow->display)
            return;

        pyWindow->display = nullptr; //refcount decrease
    }

    virtual void onResize(int w, int h, IWindow& window) override
    {
        Window* pyWindow = (Window*)window.userData();
        if (!pyWindow || !pyWindow->display)
            return;

        pyWindow->display->resize(w, h);
    }

    virtual ~ModuleWindowListener()
    {
    }

private:
    ModuleState& m_moduleState;
};

IWindowListener* Window::createWindowListener(ModuleState& module)
{
    return new ModuleWindowListener(module);
}


}
}
