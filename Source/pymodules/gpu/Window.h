#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace coalpy
{

class IWindow;

namespace gpu
{

struct Window
{
    //Data
    PyObject_HEAD
    IWindow* object;       

    //Functions
    static void makeType(PyTypeObject& outObj);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void close(PyObject* self);
    static void destroy(PyObject* self);

};


}
}
