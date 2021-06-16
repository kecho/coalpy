#include "RenderArgs.h"
#include <structmember.h>
#include <coalpy.core/Assert.h>
#include "CoalpyTypeObject.h"

namespace coalpy
{
namespace gpu
{

static PyMemberDef g_renderArgMethods[] = {
    { "renderTime", T_DOUBLE, offsetof(RenderArgs, renderTime), READONLY, "" },
    { "deltaTime",  T_DOUBLE, offsetof(RenderArgs, deltaTime),  READONLY, "" },
    { nullptr }
};

void RenderArgs::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.RenderArgs";
    t.tp_basicsize = sizeof(RenderArgs);
    t.tp_doc   = "Parameters passed in the onRender function of a window.\n";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_members = g_renderArgMethods;
}


}
}
