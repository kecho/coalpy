#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"

namespace coalpy
{

namespace gpu
{

struct Window;

struct RenderArgs
{
    //Data
    PyObject_HEAD
    double renderTime = 0.0;
    double deltaTime = 0.0;
    Window* window = nullptr;
    PyObject* imguiBuilder = nullptr;
    PyObject* implotBuilder = nullptr;
    PyObject* userData = nullptr;
    int width = 0;
    int height = 0;

    //Functions
    static const TypeId s_typeId = TypeId::RenderArgs;
    static void constructType(PyTypeObject& t);
};


}

}
