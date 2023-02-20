#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <object.h>
#include <iostream>
#include "ModuleState.h"
#include "ModuleFunctions.h"
#include "TypeRegistry.h"
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <coalpy.core/Assert.h>
#include <coalpy.files/Utils.h> 
#include <coalpy.core/ByteBuffer.h> 
#include <coalpy.core/ClTokenizer.h> 
#include <coalpy.window/WindowDefs.h>
#include <string>
#include "CoalpyTypeObject.h"

coalpy::ModuleOsHandle g_ModuleInstance = nullptr;
coalpy::gpu::TypeList g_allTypes;
std::string g_ModuleFilePath;

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
            
            #define MAX_PY_MOD_PATH 9056
            char local_ModuleFilePath[MAX_PY_MOD_PATH];
            local_ModuleFilePath[0] = '\0';
            GetModuleFileNameA(hinstDLL, local_ModuleFilePath, MAX_PY_MOD_PATH);
            g_ModuleFilePath = local_ModuleFilePath;
            break;

        case DLL_PROCESS_DETACH:
            //Cant free dx12 stuff in dllMain
            //so if we have an existing reference, we just let the graphics api 
            //deallocate for us. We will get an alive object warning message.
            //coalpy::gpu::ModuleState::clean(); 
            g_ModuleInstance = nullptr;
            for (auto t : g_allTypes)
                delete t;
            g_allTypes.clear();
            break;
    }
    return TRUE;
}

void setupModule()
{
    /*Empty*/
}

#elif defined(__linux__)

#include <dlfcn.h>

void forceLinkUse()
{
    coalpy::ByteBuffer bb;
    coalpy::FileLookup fl;
    void* fn = (void*)&coalpy::ClTokenizer::splitString;
}

void setupModule()
{
    forceLinkUse();
    Dl_info dl_info;
    dladdr((void*)setupModule, &dl_info);
    g_ModuleFilePath = dl_info.dli_fname;
}

#else
#error "Platform not supported"
#endif

static const char* s_Documentation = ""
    " << coalpy >> Compute Abstraction Layer for Python \n"
    " by Kleber A Garcia 2021 (c). \n"
    "coalpy is a compute abstraction layer for python. It brings\n"
    "advanced rendering features focused on compute shaders and an easy interface\n"
    "to create graphical applications, without dealing with graphics APIs complexity.\n"
    "Start by creating a shader, create a window, passing an onRender function, create a command list\n"
    "and start using your GPU!\n";

PyMODINIT_FUNC PyInit_gpu(void)
{
    setupModule();
    static PyModuleDef moduleDef = {
        PyModuleDef_HEAD_INIT,
        "gpu", //name
        s_Documentation,  //module documentation
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

        for (auto& childObj : t->children)
        {
            if (PyType_Ready(childObj.pyObj) < 0)
                return nullptr;

            if (PyDict_SetItemString(t->pyObj.tp_dict, childObj.name, (PyObject*)childObj.pyObj) < 0)
                return nullptr;
        }
    }

    auto state = (coalpy::gpu::ModuleState*)PyModule_GetState(moduleObj);
    new (state) coalpy::gpu::ModuleState(typeList.data(), (int)typeList.size());
    PyObject* exceptionObject = PyErr_NewException("gpu.exception_object", NULL, NULL);
    Py_XINCREF(exceptionObject);
    state->setExObj(exceptionObject);

    coalpy::gpu::processTypes(typeList, state, *moduleObj);

    if (PyModule_AddObject(moduleObj, "exception_object", exceptionObject) < 0) {
        Py_XDECREF(exceptionObject);
        Py_CLEAR(exceptionObject);
        Py_DECREF(moduleObj);
        return NULL;
    }

    {
        std::string modulePath;
        std::string moduleFilePath = g_ModuleFilePath;
        coalpy::FileUtils::getDirName(moduleFilePath, modulePath);
        state->addDataPath(modulePath.c_str());
        state->addDataPath(".");
    }

    return moduleObj;
}
