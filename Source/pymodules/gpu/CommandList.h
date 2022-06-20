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

struct CommandListReferences
{
    std::vector<PyObject*> objects;
    std::vector<render::ResourceTable> tmpTables;

    void append(const CommandListReferences& other)
    {
        objects.insert(objects.end(), other.objects.begin(), other.objects.end());
        tmpTables.insert(tmpTables.end(), other.tmpTables.begin(), other.tmpTables.end());
    }
};

struct CoalpyTypeObject;

struct CommandList
{
    //Data
    PyObject_HEAD

    render::CommandList* cmdList;
    CommandListReferences references;

    //Functions
    static const TypeId s_typeId = TypeId::CommandList;
    static void constructType(CoalpyTypeObject& o);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}
}
