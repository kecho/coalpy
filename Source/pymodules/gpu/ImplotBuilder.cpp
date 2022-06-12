#include "ImplotBuilder.h"
#include "HelperMacros.h"
#include "PyUtils.h"
#include "TypeIds.h"
#include "CoalpyTypeObject.h"
#include "Resources.h"
#include <functional>
#include <cpp/imgui_stdlib.h>

namespace coalpy
{

namespace gpu
{

namespace methods
{
    #include "bindings/MethodDecl.h"
    #include "bindings/Implot.inl"
}

static PyMethodDef g_implotMethods[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/Implot.inl"
    FN_END
};

void ImplotBuilder::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.ImplotBuilder";
    t.tp_basicsize = sizeof(ImplotBuilder);
    t.tp_doc   = R"(
    Implot builder class. Use this class' methods to build a Dear Implot on a specified window.
    To utilize you need to instantiate a Window object. In its constructor set use_imgui to True (which is by default).
    On the on_render function, you will receive a RenderArgs object. There will be a ImplotBuilder object in the implot member that you can now
    query to construct your plots.
    Coalpy does not support creation of a ImplotBuilder object, it will always be passed as an argument on the RenderArgs of the window.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = ImplotBuilder::init;
    t.tp_dealloc = ImplotBuilder::destroy;
    t.tp_methods = g_implotMethods;
}

int ImplotBuilder::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* obj = (ImplotBuilder*)self;
    new (obj) ImplotBuilder;
    return 0;
}

void ImplotBuilder::destroy(PyObject* self)
{
    ((ImplotBuilder*)self)->~ImplotBuilder();
    Py_TYPE(self)->tp_free(self);
}

namespace methods
{

PyObject* showDemoWindow(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ImPlot::ShowDemoWindow();
    Py_RETURN_NONE;
}

}

}
}
