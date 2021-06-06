#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include "ModuleState.h"
#include "ModuleFunctions.h"
#include "TypeRegistry.h"
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <coalpy.core/Assert.h>
#include <coalpy.window/WindowDefs.h>
#include "CoalpyTypeObject.h"


coalpy::ModuleOsHandle g_ModuleInstance = nullptr;
coalpy::gpu::TypeList g_allTypes;

#if _WIN32

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
            g_ModuleInstance = (coalpy::ModuleOsHandle)hinstDLL;
            break;

        case DLL_PROCESS_DETACH:
            g_ModuleInstance = nullptr;
            for (auto t : g_allTypes)
                delete t;
            g_allTypes.clear();
            break;
    }
    return TRUE;
}

#endif

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

    coalpy::gpu::TypeList typeList;
    coalpy::gpu::constructTypes(typeList);
    g_allTypes.insert(g_allTypes.end(), typeList.begin(), typeList.end());

    //Check types
    for (auto& t : typeList)
    {
        if (PyType_Ready(&t->pyObj) < 0)
            return nullptr;
    }

    PyObject* moduleObj = PyModule_Create(&moduleDef);

    //Register types
    int prependStrLen = strlen("gpu.");
    for (auto& t : typeList)
    {
        int typeNameLen = strlen(t->pyObj.tp_name);
        CPY_ASSERT_FMT(typeNameLen > prependStrLen, "typename %s must have prefix \"gpu.\"", t->pyObj.tp_name);
        if (typeNameLen <= prependStrLen)
            return nullptr;

        Py_INCREF(&t->pyObj);
        const char* typeName = t->pyObj.tp_name + prependStrLen;
        if (PyModule_AddObject(moduleObj, typeName, (PyObject*)&t->pyObj) < 0)
        {
            Py_DECREF(&t->pyObj);
            Py_DECREF(moduleObj);
            return nullptr;
        }
    }

    auto state = (coalpy::gpu::ModuleState*)PyModule_GetState(moduleObj);
    new (state) coalpy::gpu::ModuleState(typeList.data(), (int)typeList.size());
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
