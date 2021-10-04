#include "RenderArgs.h"
#include <structmember.h>
#include <coalpy.core/Assert.h>
#include "CoalpyTypeObject.h"

namespace coalpy
{
namespace gpu
{

static PyMemberDef g_renderArgMemembers[] = {
    { "render_time", T_DOUBLE, offsetof(RenderArgs, renderTime), READONLY, "Total rendering time from the start of this application, in milliseconds" },
    { "delta_time",  T_DOUBLE, offsetof(RenderArgs, deltaTime),  READONLY, "Delta time from previous frame in milliseconds" },
    { "window", T_OBJECT, offsetof(RenderArgs, window), READONLY, "The window object being rendered." },
    { "user_data", T_OBJECT, offsetof(RenderArgs, userData), READONLY, "Custom user_data object set in the Window object. You can store here your view user data such as texture / buffers / shaders." },
    { "width",  T_INT, offsetof(RenderArgs, width), READONLY, "The current width of this window." },
    { "height", T_INT, offsetof(RenderArgs, height), READONLY, "The current height of this window." },
    { "imgui", T_OBJECT, offsetof(RenderArgs, imguiBuilder), READONLY, "The ImguiBuilder object, used to build a Dear Imgui. For more info read on Window constructor,  ImguiBuilder and its methods." },
    { nullptr }
};

void RenderArgs::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.RenderArgs";
    t.tp_basicsize = sizeof(RenderArgs);
    t.tp_doc   = "Parameters passed in the on_render function of a window.\n";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_members = g_renderArgMemembers;
}


}
}
