#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include "ModuleState.h"
#include "ModuleFunctions.h"

PyMODINIT_FUNC PyInit_gpu(void)
{
    static PyModuleDef moduleDef = {
        PyModuleDef_HEAD_INIT,
        "gpu", //name
        nullptr,  //module documentation
        sizeof(coalpy::gpu::ModuleState), //size of state object
        nullptr, //methods
        nullptr, //slots
        nullptr, //traverse proc
        nullptr, //clear proc
        coalpy::gpu::methods::freeModule
    };

    moduleDef.m_methods = coalpy::gpu::methods::get();

    PyObject* moduleObj = PyModule_Create(&moduleDef);
    auto state = (coalpy::gpu::ModuleState*)PyModule_GetState(moduleObj);
    new (state) coalpy::gpu::ModuleState;
    state->startServices();

    PyObject* exceptionObject = PyErr_NewException("gpu.exceptionObject", NULL, NULL);
    Py_XINCREF(exceptionObject);
    state->setExObj(exceptionObject);

    if (PyModule_AddObject(moduleObj, "exceptionObject", exceptionObject) < 0) {
        Py_XDECREF(exceptionObject);
        Py_CLEAR(exceptionObject);
        Py_DECREF(moduleObj);
        return NULL;
    }

    return moduleObj;
}
