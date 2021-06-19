#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.window/WindowDefs.h>
#include <coalpy.core/SmartPtr.h>
#include "TypeIds.h"

namespace coalpy
{

namespace render
{
    class IDisplay;
}

class IWindow;

namespace gpu
{

class ModuleState;

struct Window
{
    //Data
    PyObject_HEAD
    PyObject* onRenderCallback;
    IWindow*  object;
    SmartPtr<render::IDisplay> display;

    //Functions
    static const TypeId s_typeId = TypeId::Window;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void close(PyObject* self);
    static void destroy(PyObject* self);
    static IWindowListener* createWindowListener(ModuleState& moduleState);

};


}
}
