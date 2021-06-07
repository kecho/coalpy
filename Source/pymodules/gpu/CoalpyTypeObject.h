#pragma once

#include "ModuleState.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace coalpy
{
namespace gpu
{

class ModuleState;

struct CoalpyTypeObject
{
    //Must be the first member, offset 0
    PyTypeObject pyObj;
    ModuleState* moduleState;
    TypeId typeId;
}; 

static_assert(offsetof(CoalpyTypeObject, pyObj) == 0);

inline CoalpyTypeObject* asCoalpyObj(PyTypeObject* obj)
{
    return reinterpret_cast<CoalpyTypeObject*>(obj);
}

inline ModuleState& parentModule(PyObject* object)
{
    return *(asCoalpyObj(Py_TYPE(object))->moduleState);
}

}
}
