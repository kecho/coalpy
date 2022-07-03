#include "ImplotBuilder.h"
#include "HelperMacros.h"
#include "PyUtils.h"
#include "TypeIds.h"
#include "CoalpyTypeObject.h"
#include "Resources.h"
#include <functional>
#include <cpp/imgui_stdlib.h>
#include <iostream>

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

void ImplotBuilder::constructType(CoalpyTypeObject& o)
{
    auto& t = o.pyObj;
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

bool CheckImplot(PyObject* builder)
{
    auto& implotBuilder = *(ImplotBuilder*)builder;
    if (implotBuilder.enabled)
        return true;

    ModuleState& moduleState = parentModule(builder);
    PyErr_SetString(moduleState.exObj(), "Cannot use the imgui object outside of a on_render callback. Ensure you create a window, and use the imgui object provided from the renderArgs.");
    return false;
}

#define CHECK_IMPLOT \
    if (!CheckImplot(self))\
        return nullptr;


PyObject* showDemoWindow(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    ImPlot::ShowDemoWindow();
    Py_RETURN_NONE;
}

PyObject* beginPlot(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    char* name = nullptr;
    ImVec2 size = ImVec2(-1, 0);
    int flags = 0;
    static char* argnames[] = { "name", "size", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|(ff)i", argnames, &name, &size, &flags))
        return nullptr;
    if (ImPlot::BeginPlot(name, size, flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* endPlot(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    ImPlot::EndPlot();
    Py_RETURN_NONE;
}


PyObject* setupAxes(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    char* x_label = nullptr;
    char* y_label = nullptr;
    int x_flags = 0;
    int y_flags = 0;
    static char* argnames[] = { "x_label", "y_label", "x_flags", "y_flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "ss|ii", argnames, &x_label, &y_label, &x_flags, &y_flags))
        return nullptr;

    ImPlot::SetupAxes(x_label, y_label, x_flags, y_flags);

    Py_RETURN_NONE;
}

PyObject* setupAxisLimits(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    int axis = 0;
    double v_min = 0.0;
    double v_max = 0.0;
    int cond = ImPlotCond_Once;

    static char* argnames[] = { "axis", "v_min", "v_max", "cond", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "iddi", argnames, &axis, &v_min, &v_max, &cond))
        return nullptr;

    ImPlot::SetupAxisLimits(axis, v_min, v_max, cond);
    Py_RETURN_NONE;
}

static bool openVec2Buffer(ModuleState& moduleState, PyObject* values, int offset, int count, Py_buffer& valuesView)
{
    valuesView = {};
    if (PyObject_GetBuffer(values, &valuesView, 0) == -1)
    {
        PyErr_SetString(moduleState.exObj(), "Failed retrieving buffer contents of buffer object values.");
        return false;
    }

    if (valuesView.itemsize != sizeof(float) && valuesView.itemsize != sizeof(double))
    {
        PyBuffer_Release(&valuesView);
        PyErr_SetString(moduleState.exObj(), "Values array object passed to plot line must be internally contiguous float or double.");
        return false;
    }

    int elementCount = (valuesView.len / valuesView.itemsize);
    if (elementCount < 2 * count || offset * 2 > elementCount)
    {
        int itemOffset = offset * 2;
        int itemCount = count * 2;
        PyErr_Format(moduleState.exObj(), "Trying to access values array out of bounds. Max count is %d, accessing with offset %d and count %d", elementCount, itemOffset, itemCount);
        PyBuffer_Release(&valuesView);
        return false;
    }

    return true;
}

PyObject* plotLine(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    char* label = nullptr;
    PyObject* values = nullptr;
    int count = 0;
    int offset = 0;

    static char* argnames[] = { "label", "values", "count", "offset", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sOii", argnames, &label, &values, &count, &offset))
        return nullptr;

    ModuleState& moduleState = parentModule(self);
    Py_buffer valuesView = {};
    if (!openVec2Buffer(moduleState, values, offset, count, valuesView))
        return nullptr;

    if (valuesView.itemsize == sizeof(float))
    {
        float* values = (float*)valuesView.buf;
        ImPlot::PlotLine(label, values, values + 1, count, offset, 2 * sizeof(float));
    }
    else if (valuesView.itemsize == sizeof(double))
    {
        double* values = (double*)valuesView.buf;
        ImPlot::PlotLine(label, values, values + 1, count, offset, 2 * sizeof(double));
    }

    PyBuffer_Release(&valuesView);
    Py_RETURN_NONE;
}

PyObject* plotShaded(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMPLOT
    char* label = nullptr;
    PyObject* values = nullptr;
    float yref;
    int count = 0;
    int offset = 0;

    static char* argnames[] = { "label", "values", "count", "yref", "offset", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sOifi", argnames, &label, &values, &count, &yref, &offset))
        return nullptr;

    ModuleState& moduleState = parentModule(self);
    Py_buffer valuesView = {};
    if (!openVec2Buffer(moduleState, values, offset, count, valuesView))
        return nullptr;

    if (valuesView.itemsize == sizeof(float))
    {
        float* values = (float*)valuesView.buf;
        ImPlot::PlotShaded(label, values, values + 1, count, yref, offset, 2 * sizeof(float));
    }
    else if (valuesView.itemsize == sizeof(double))
    {
        double* values = (double*)valuesView.buf;
        ImPlot::PlotShaded(label, values, values + 1, count, (double)yref, offset, 2 * sizeof(double));
    }

    PyBuffer_Release(&valuesView);
    Py_RETURN_NONE;
}

}
}
}
