#include "RenderArgs.h"
#include <structmember.h>
#include <coalpy.core/Assert.h>
#include "CoalpyTypeObject.h"

namespace coalpy
{
namespace gpu
{

static PyMemberDef g_renderArgMemembers[] = {
    { "render_time", T_DOUBLE, offsetof(RenderArgs, renderTime), READONLY, "" },
    { "delta_time",  T_DOUBLE, offsetof(RenderArgs, deltaTime),  READONLY, "" },
    { "window", T_OBJECT, offsetof(RenderArgs, window), READONLY, "" },
    { "user_data", T_OBJECT, offsetof(RenderArgs, userData), READONLY, "" },
    { "width",  T_INT, offsetof(RenderArgs, width), READONLY, "" },
    { "height", T_INT, offsetof(RenderArgs, height), READONLY, "" },
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
