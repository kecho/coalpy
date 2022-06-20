#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"

namespace coalpy
{
namespace gpu
{

struct CoalpyTypeObject;

struct TinyObjLoader
{
    //Data
    PyObject_HEAD

    //Functions
    static const TypeId s_typeId = TypeId::TinyObjLoader;
    static void constructType(CoalpyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}
}
