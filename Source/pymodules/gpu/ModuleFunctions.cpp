#include "ModuleFunctions.h"
#include "ModuleState.h"
#include "HelperMacros.h"
#include <coalpy.core/Assert.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.window/IWindow.h>
#include <string>
#include "Window.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace {

PyMethodDef g_defs[] = {
    KW_FN(loadShader,   "Loads a shader from a file (fileName: string, [mainFunction: string], [identifier: string]"),
    KW_FN(inlineShader, "Create an inline shader"),
    VA_FN(run, "Runs window rendering callbacks. This function blocks until all the existing windows are closed."),
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

PyObject* run(PyObject* self, PyObject* args)
{
    ModuleState& state = getState(self);

    WindowRunArgs runArgs = {};
    bool raiseException = false;
    runArgs.onRender = [&state, &raiseException]()
    {
        std::set<Window*> windowsPtrs;
        state.getWindows(windowsPtrs);
        for (Window* w : windowsPtrs)
        {
            CPY_ASSERT(w != nullptr);
            if (w == nullptr)
                return false;
            if (w && w->onRenderCallback != nullptr)
            {
                PyObject* retObj = PyObject_CallObject(w->onRenderCallback, nullptr);
                //means an exception has been risen. Propagate up.
                if (retObj == nullptr)
                {
                    raiseException = true;
                    return false;
                }
                Py_DECREF(retObj);
            }
        }

        return true;
    };

    IWindow::run(runArgs); //block
    if (raiseException)
        return nullptr; //means we propagate an exception.

    Py_RETURN_NONE;
}

}
}
}
