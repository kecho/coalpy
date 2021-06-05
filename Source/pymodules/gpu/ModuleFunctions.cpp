#include "ModuleFunctions.h"
#include "ModuleState.h"
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.window/IWindow.h>
#include <string>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace {

#define KW_FN(name, desc) \
    { #name, (PyCFunction)(coalpy::gpu::methods::##name), METH_VARARGS | METH_KEYWORDS, desc }

#define VA_FN(name, desc) \
    { #name, (coalpy::gpu::methods::##name), METH_VARARGS, desc }

#define FN_END \
    {NULL, NULL, 0, NULL}

PyMethodDef g_defs[] = {
    KW_FN(loadShader,   "Loads a shader from a file (fileName: string, [mainFunction: string], [identifier: string]"),
    KW_FN(inlineShader, "Create an inline shader"),
    VA_FN(runWindows, "Runs window rendering callbacks. This function blocks until all the windows specified are closed."),
    FN_END
};

coalpy::gpu::ModuleState& getState(PyObject* module)
{
    return *((coalpy::gpu::ModuleState*)PyModule_GetState(module));
}

}

namespace coalpy
{
namespace gpu
{
namespace methods
{

PyMethodDef* get()
{
    return g_defs;
}

void freeModule(void* modulePtr)
{
    PyObject* moduleObj = (PyObject*)modulePtr;
    auto state = (ModuleState*)PyModule_GetState(moduleObj);
    if (state)
        state->~ModuleState();
}

PyObject* loadShader(PyObject* self, PyObject* args, PyObject* kwds)
{
    static char* argnames[] = { "fileName", "mainFunction", "identifier" };
    ModuleState& state = getState(self);
    const char* fileName = nullptr;
    const char* mainFunction = "csMain";
    const char* shaderName = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|ss", argnames, &fileName, &mainFunction, &shaderName))
    {
        return nullptr;
    }

    std::string sshaderName = shaderName ? shaderName : "";
    if (sshaderName == "")
    {
        std::string filePath = fileName;
        FileUtils::getFileName(filePath, sshaderName);
    }
    
    ShaderDesc desc;
    desc.type = ShaderType::Compute;
    desc.name = sshaderName.c_str();
    desc.mainFn = mainFunction;
    desc.path = fileName;
    ShaderHandle handle = state.db().requestCompile(desc);
    Py_RETURN_NONE;
}

PyObject* inlineShader(PyObject* self, PyObject* args, PyObject* kwds)
{
    Py_RETURN_NONE;
}

PyObject* runWindows(PyObject* self, PyObject* args)
{
    ModuleState& state = getState(self);
    PyObject* renderCb;
    if (!PyArg_ParseTuple(args, "O:runWindows", &renderCb))
        return nullptr;

    if (!PyCallable_Check(renderCb))
    {
        PyErr_SetString(state.exObj(), "parameter must be a callback");
        return nullptr;
    }

    Py_XINCREF(renderCb);

    WindowRunArgs runArgs = {};
    runArgs.onRender = [renderCb](IWindow* window)
    {
        PyObject_CallObject(renderCb, nullptr);
    };

    IWindow::run(runArgs);

    Py_XDECREF(renderCb);
    Py_RETURN_NONE;
}

}
}
}
