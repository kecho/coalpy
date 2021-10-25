#include "ImguiBuilder.h"
#include "HelperMacros.h"
#include "PyUtils.h"
#include "TypeIds.h"
#include "CoalpyTypeObject.h"
#include <functional>
#include <imgui.h>
#include <cpp/imgui_stdlib.h>

namespace coalpy
{

namespace gpu
{

namespace methods
{
#define IMGUI_FN(pyname, cppname, doc) static PyObject* cppname(PyObject* self, PyObject* vargs, PyObject* kwds);
    #include "ImguiFunctions.inl"
#undef  IMGUI_FN
}

static PyMethodDef g_imguiMethods[] = {
#define IMGUI_FN(pyname, cppname, doc) KW_FN(pyname, cppname, doc),
    #include "ImguiFunctions.inl"
#undef  IMGUI_FN
    FN_END
};

void ImguiBuilder::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.ImguiBuilder";
    t.tp_basicsize = sizeof(ImguiBuilder);
    t.tp_doc   = R"(
    Imgui builder class. Use this class' methods to build a Dear ImGUI on a specified window.
    To utilize you need to instantiate a Window object. In its constructor set use_imgui to True (which is by default).
    On the on_render function, you will receive a RenderArgs object. There will be a ImguiBuilder object in the imgui member that you can now
    query to construct your user interface.
    Coalpy does not support creation of a ImguiBuilder object, it will always be passed as an argument on the RenderArgs of the window.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = ImguiBuilder::init;
    t.tp_dealloc = ImguiBuilder::destroy;
    t.tp_methods = g_imguiMethods;
}

int ImguiBuilder::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* obj = (ImguiBuilder*)self;
    new (obj) ImguiBuilder;
    return 0;
}

void ImguiBuilder::destroy(PyObject* self)
{
    ((ImguiBuilder*)self)->~ImguiBuilder();
    Py_TYPE(self)->tp_free(self);
}


/*** Imgui API Wrappers***/
namespace methods
{

bool CheckImgui(PyObject* builder)
{
    auto& imguiBuilder = *(ImguiBuilder*)builder;
    if (imguiBuilder.enabled)
        return true;

    ModuleState& moduleState = parentModule(builder);
    PyErr_SetString(moduleState.exObj(), "Cannot use the imgui object outside of a on_render callback. Ensure you create a window, and use the imgui object provided from the renderArgs.");
    return false;
}

#define CHECK_IMGUI \
    if (!CheckImgui(self))\
        return nullptr;

PyObject* begin(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* name = nullptr;
    int is_openint = 1;
    static char* argnames[] = { "name", "is_open", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|p", argnames, &name, &is_openint))
        return nullptr;

    bool is_open = is_openint ? true : false;

    ImGui::Begin(name, &is_open);
    if (is_open)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* end(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::End();
    Py_RETURN_NONE;
}

PyObject* showDemoWindow(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::ShowDemoWindow();
    Py_RETURN_NONE;
}

PyObject* sliderFloat(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    float v, v_min, v_max;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "v_min", "v_max", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sfff|s", argnames, &label, &v, &v_min, &v_max, &fmt))
        return nullptr;
    
    ImGui::SliderFloat(label, &v, v_min, v_max, fmt);
    return PyFloat_FromDouble((double)v);
}

PyObject* inputFloat(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    float v;
    float step = 0.0f, step_fast = 0.0f;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "step", "step_fast", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sf|ffs", argnames, &label, &v, &step, &step_fast, &fmt))
        return nullptr;
    
    ImGui::InputFloat(label, &v, step, step_fast, fmt);
    return PyFloat_FromDouble((double)v);
}

PyObject* inputFloat2(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    CHECK_IMGUI
    char* label;
    PyObject* v;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|s", argnames, &label, &v, &fmt))
        return nullptr;

    float vecVal[2];
    if (!getTupleValuesFloat(v, vecVal, 2, 2)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse v value. Must be a tuple or list of length 2");
        return nullptr;
    }
    
    ImGui::InputFloat2(label, vecVal, fmt);
    return Py_BuildValue("(ff)", vecVal[0], vecVal[1]);
}

PyObject* inputFloat3(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    CHECK_IMGUI
    char* label;
    PyObject* v;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|s", argnames, &label, &v, &fmt))
        return nullptr;

    float vecVal[3];
    if (!getTupleValuesFloat(v, vecVal, 3, 3)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse v value. Must be a tuple or list of length 3");
        return nullptr;
    }
    
    ImGui::InputFloat3(label, vecVal, fmt);
    return Py_BuildValue("(fff)", vecVal[0], vecVal[1], vecVal[2]);
}

PyObject* inputFloat4(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    CHECK_IMGUI
    char* label;
    PyObject* v;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|s", argnames, &label, &v, &fmt))
        return nullptr;

    float vecVal[4];
    if (!getTupleValuesFloat(v, vecVal, 4, 4)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse v value. Must be a tuple or list of length 4");
        return nullptr;
    }
    
    ImGui::InputFloat4(label, vecVal, fmt);
    return Py_BuildValue("(ffff)", vecVal[0], vecVal[1], vecVal[2], vecVal[3]);
}

PyObject* inputText(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    char* inputStr;
    static char* argnames[] = { "label", "v", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "ss", argnames, &label, &inputStr))
        return nullptr;

    std::string str = inputStr;
    ImGui::InputText(label, &str);
    return Py_BuildValue("s", str.c_str());
}

PyObject* text(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* text;
    static char* argnames[] = { "text", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &text))
        return nullptr;

    ImGui::Text("%s", text);
    Py_RETURN_NONE;
}

PyObject* pushId(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* text;
    static char* argnames[] = { "text", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &text))
        return nullptr;

    ImGui::PushID("%s");
    Py_RETURN_NONE;
}

PyObject* popId(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* text;
    ImGui::PopID();
    Py_RETURN_NONE;
}

PyObject* collapsingHeader(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    static char* argnames[] = { "label", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &label))
        return nullptr;

    bool ret = ImGui::CollapsingHeader(label);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* beginMainMenuBar(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    if (ImGui::BeginMainMenuBar())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

PyObject* endMainMenuBar(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndMainMenuBar();
    Py_RETURN_NONE;
}

PyObject* beginMenu(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    static char* argnames[] = { "label", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &label))
        return nullptr;

    bool ret = ImGui::BeginMenu(label);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* menuItem(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    char* shortcut = nullptr;
    int enabledint = 1;
    static char* argnames[] = { "label", "shortcut", "enabled", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|sp", argnames, &label, &shortcut, &enabledint))
        return nullptr;

    bool ret = ImGui::MenuItem(label, shortcut, false, (bool)enabledint);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* endMenu(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndMenu();
    Py_RETURN_NONE;
}

PyObject* button(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    static char* argnames[] = { "label", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &label))
        return nullptr;

    bool ret = ImGui::Button(label);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* checkbox(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    int vint;
    static char* argnames[] = { "label", "v", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sp", argnames, &label, &vint))
        return nullptr;

    bool v = (bool)vint;
    bool ret = ImGui::Checkbox(label, &v);
    if (v)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

}

}

}
