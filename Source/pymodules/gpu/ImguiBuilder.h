#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.core/SmartPtr.h>
#include <set>
#include <imgui.h>

namespace coalpy
{

namespace gpu
{

struct Texture;

struct ImguiBuilder
{
    //Data
    PyObject_HEAD

    bool enabled = false;
    SmartPtr<render::IimguiRenderer> activeRenderer;
    std::set<Texture*> textureReferences;

    //Functions
    static const TypeId s_typeId = TypeId::ImguiBuilder;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);

    //Helpers
    ImTextureID getTextureID(Texture* texture);
    void clearTextureReferences();


};

}

}
