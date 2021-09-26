#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <coalpy.render/CommandDefs.h>
#include <coalpy.render/Resources.h>

namespace coalpy
{
namespace gpu
{

struct ResourceDownloadRequest
{
    //Data
    PyObject_HEAD
    render::WorkHandle workHandle;
    render::ResourceHandle resource;
    int mipLevel = 0;
    int sliceIndex = 0;
    bool resolved = false;

    PyObject* resourcePyObj = nullptr;

    PyObject* dataAsByteArray = nullptr;
    PyObject* rowBytesPitchObject = nullptr;

    //Functions
    static const TypeId s_typeId = TypeId::ResourceDownloadRequest;
    static void constructType(PyTypeObject& t);
    static int  init(PyObject* self, PyObject * vargs, PyObject* kwds);
    static void destroy(PyObject* self);
};

}
}
