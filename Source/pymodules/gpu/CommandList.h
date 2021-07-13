#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <coalpy.render/CommandList.h>

namespace coalpy
{
namespace gpu
{

struct CommandList
{
    //Data
    PyObject_HEAD

    //Functions
    static const TypeId s_typeId = TypeId::CommandList;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}
}
