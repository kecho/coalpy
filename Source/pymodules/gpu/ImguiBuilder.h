#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"

namespace coalpy
{

namespace gpu
{

struct ImguiBuilder
{
    //Data
    PyObject_HEAD

    bool enabled = false;

    //Functions
    static const TypeId s_typeId = TypeId::ImguiBuilder;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);

};

}

}
