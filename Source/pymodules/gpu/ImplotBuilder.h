#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.core/SmartPtr.h>
#include <set>
#include <implot.h>

namespace coalpy
{

namespace gpu
{

struct Texture;

struct ImplotBuilder
{
    //Data
    PyObject_HEAD

    bool enabled = false;
    
    SmartPtr<render::IimguiRenderer> activeRenderer;

    //Functions
    static const TypeId s_typeId = TypeId::ImplotBuilder;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}

}
