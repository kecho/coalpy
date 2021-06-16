#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"

namespace coalpy
{

namespace gpu
{


struct RenderArgs
{
    //Data
    PyObject_HEAD

    //Functions
    static const TypeId s_typeId = TypeId::RenderArgs;
    static void constructType(PyTypeObject& t);
};


}

}
