#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"

namespace coalpy
{

class IWindow;

namespace gpu
{

struct Window
{
    //Data
    PyObject_HEAD
    PyObject* onRenderCallback;
    IWindow* object;

    //Functions
    static const TypeId s_typeId = TypeId::Window;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void close(PyObject* self);
    static void destroy(PyObject* self);

};


}
}
