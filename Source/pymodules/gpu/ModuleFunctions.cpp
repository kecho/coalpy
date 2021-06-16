#include "ModuleFunctions.h"
#include "ModuleState.h"
#include "HelperMacros.h"
#include <string>
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include <coalpy.render/IShaderDb.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace {

PyMethodDef g_defs[] = {

    KW_FN(
        inlineShader,
        "Create an inline shader. Arguments:\n"
        "name: Name of shader. It is mandatory for inline shaders.\n"
        "source: Source code of shader. Use HLSL.\n"
        "mainFunction (optional): entry point of shader. Default is 'main'.\n"
    ),

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

PyObject* inlineShader(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& state = getState(self);
    const char* shaderName = nullptr;
    const char* shaderSource = nullptr;
    const char* mainFunction = "main";

    static char* argnames[] = { "name", "source", "mainFunction", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "ss|s", argnames, &shaderName, &shaderSource, &mainFunction))
        return nullptr;

    PyObject* obj = PyType_GenericAlloc(state.getType(TypeId::Shader), 1); 
    Shader* shaderObj = reinterpret_cast<Shader*>(obj);

    ShaderInlineDesc desc;
    desc.type = ShaderType::Compute;
    desc.mainFn = mainFunction;
    desc.name = shaderName;
    desc.immCode = shaderSource;
    shaderObj->db = &state.db();
    shaderObj->handle = state.db().requestCompile(desc);
    return obj;
}

PyObject* run(PyObject* self, PyObject* args)
{
    ModuleState& state = getState(self);

    //prepare arguments for this run call.
    RenderArgs* renderArgs = state.alloc<RenderArgs>();
    renderArgs->renderTime = 99.0;
    renderArgs->deltaTime = 101.0;
    
    WindowRunArgs runArgs = {};
    bool raiseException = false;

    runArgs.onRender = [&state, &raiseException, renderArgs]()
    {
        std::set<Window*> windowsPtrs;
        state.getWindows(windowsPtrs);
        int openedWindows = 0;
        for (Window* w : windowsPtrs)
        {
            CPY_ASSERT(w != nullptr);
            if (w == nullptr)
                return false;

            if (w->object->isClosed())
                continue;

            ++openedWindows;
            if (w && w->onRenderCallback != nullptr)
            {
                PyObject* retObj = PyObject_CallOneArg(w->onRenderCallback, (PyObject*)renderArgs);
                //means an exception has been risen. Propagate up.
                if (retObj == nullptr)
                {
                    raiseException = true;
                    return false;
                }
                Py_DECREF(retObj);
            }

        }

        return openedWindows != 0;
    };

    IWindow::run(runArgs); //block
    if (raiseException)
    {
        Py_DECREF(renderArgs);
        return nullptr; //means we propagate an exception.
    }

    Py_DECREF(renderArgs);
    Py_RETURN_NONE;
}

}
}
}
