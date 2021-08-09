#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <string>

namespace coalpy
{

namespace gpu
{

class ModuleState;

namespace methods
{

PyMethodDef* get();

//Module functions
void freeModule(void* modulePtr); 

// utilities
bool getModulePath(ModuleState& moduleState, PyObject* moduleObject, std::string& path);

// coalpy::gpu
PyObject* loadShader(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* inlineShader(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* getAdapters(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* getCurrentAdapterInfo(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* setCurrentAdapter(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* addDataPath(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* schedule(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* run(PyObject* self, PyObject* args);

}

}

}
