#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <coalpy.render/ShaderDefs.h>

namespace coalpy
{

class IShaderDb;

namespace gpu
{

struct CoalpyTypeObject;

struct Shader
{
    //Data
    PyObject_HEAD
    ShaderHandle handle;
    IShaderDb* db;

    //Functions
    static const TypeId s_typeId = TypeId::Shader;
    static void constructType(CoalpyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}
}

