#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <coalpy.render/CommandList.h>
#include <vector>

namespace coalpy
{

namespace render
{
class CommandList;
}

namespace gpu
{

struct CommandList
{
    //Data
    PyObject_HEAD

    render::CommandList* cmdList;
    std::vector<PyObject*> references;

    //Functions
    static const TypeId s_typeId = TypeId::CommandList;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}
}
