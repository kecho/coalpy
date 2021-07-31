#include "Window.h"
#include "Resources.h"
#include <iostream>
#include <structmember.h>
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.render/IDevice.h>
#include "CoalpyTypeObject.h"

extern coalpy::ModuleOsHandle g_ModuleInstance;

namespace coalpy
{
namespace gpu
{


static PyMemberDef g_windowMembers[] = {
    { "display_texture", T_OBJECT, offsetof(Window, displayTexture), READONLY, "" },
    { "user_data", T_OBJECT, offsetof(Window, userData), 0, "" },
    { nullptr }
};

void Window::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Window";
    t.tp_basicsize = sizeof(Window);
    t.tp_doc   = "Class that represnts a window.\n"
                 "Constructor Arguments:\n"
                 "title  : String title for the window.\n"
                 "width  : initial width of window and swap chain texture.\n"
                 "height : initial height of window and swap chain texture.\n"
                 "on_render : Rendering function. The function has 1 argument of type RenderArgs and no return. See RenderArgs for more info.\n"
                 "use_imgui: (True by default), set to true, and during onRender the renderArgs object will contain an imgui object. Use this object to render imgui into the window.\n";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Window::init;
    t.tp_members = g_windowMembers;
    t.tp_dealloc = Window::destroy;
}

int Window::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto& window = *(Window*)self;
    new (&window) Window;
    window.userData = Py_None;
    Py_INCREF(window.userData);
    window.object = nullptr;
    window.onRenderCallback = nullptr;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    const char* windowTitle = "coalpy.gpu.window";
    WindowDesc desc;
    desc.osHandle = g_ModuleInstance;
    desc.width = 400;
    desc.height = 400;
    window.onRenderCallback = nullptr;

    int useImgui = 1;

    static char* keywords[] = { "title", "width", "height", "on_render", "use_imgui", nullptr };
    if (!PyArg_ParseTupleAndKeywords(
            vargs, kwds, "|siiOb:Window", keywords,
            &windowTitle, &desc.width, &desc.height, &window.onRenderCallback, &useImgui))
    {
        return -1;
    }

    desc.title = windowTitle;

    Py_XINCREF(window.onRenderCallback);
    if (window.onRenderCallback != nullptr && !PyCallable_Check(window.onRenderCallback))
    {
        PyErr_SetString(moduleState.exObj(), "on_render argument must be a callback function receiving 1 argument of type coalpy.gpy.RenderArgs");
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

        window.displayTexture = moduleState.alloc<Texture>();
        window.displayTexture->owned = false;
        window.displayTexture->texture = window.display->texture();

    }

    if (useImgui)
    {
        render::IimguiRendererDesc desc;
        desc.device = &moduleState.device();
        desc.display = window.display;
        desc.window = window.object;
        window.uiRenderer = render::IimguiRenderer::create(desc);
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
    auto w = (Window*)self;
    w->displayTexture->texture = render::Texture(); //invalidate texture
    w->uiRenderer = nullptr;
    w->display = nullptr;
    Py_DECREF(w->displayTexture);
    Py_DECREF(w->userData);
    w->~Window();
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

        pyWindow->uiRenderer = nullptr;
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
