#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <coalpy.render/IDisplay.h>
#include <coalpy.render/IimguiRenderer.h>
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

struct CoalpyTypeObject;
class ModuleState;
class Texture;

struct Window
{
    //Data
    PyObject_HEAD
    PyObject* onRenderCallback;
    IWindow*  object;
    PyObject* userData;
    SmartPtr<render::IDisplay> display;
    SmartPtr<render::IimguiRenderer> uiRenderer;
    Texture* displayTexture;

    //Functions
    static const TypeId s_typeId = TypeId::Window;
    static void constructType(CoalpyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void close(PyObject* self);
    static void destroy(PyObject* self);
    static IWindowListener* createWindowListener(ModuleState& moduleState);
};


}
}
