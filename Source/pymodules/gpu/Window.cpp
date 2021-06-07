#include "Window.h"
#include <iostream>
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
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
                 "width  : initial width of window and swap chain texture.\n"
                 "height : initial height of window and swap chain texture.\n"
                 "renderFunction : Rendering function. The function has 1 argument of type RenderArgs and no return. See RenderArgs for more info.\n";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Window::init;
    t.tp_dealloc = Window::destroy;
}

int Window::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto& window = *(Window*)self;
    WindowDesc desc;
    desc.osHandle = g_ModuleInstance;
    desc.width = 400;
    desc.height = 400;
    window.onRenderCallback = nullptr;

    static char* keywords[] = { "width", "height", "renderFunction", nullptr };
    if (!PyArg_ParseTupleAndKeywords(
            vargs, kwds, "|iiO:Window", keywords,
            &desc.width, &desc.height, &window.onRenderCallback))
    {
        return -1;
    }

    Py_XINCREF(window.onRenderCallback);
    if (!PyCallable_Check(window.onRenderCallback))
    {
        Py_XDECREF(window.onRenderCallback);
        return -1;
    }

    window.object = IWindow::create(desc);
    window.object->setUserData(self);
    {
        ModuleState& moduleState = parentModule(self);
        moduleState.registerWindow(&window);
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


}
}
