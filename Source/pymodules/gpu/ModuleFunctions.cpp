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
#include <coalpy.render/IDevice.h>
#include <coalpy.core/Stopwatch.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace {

PyMethodDef g_defs[] = {

    KW_FN(
        inline_shader,
        inlineShader,
        "Create an inline shader. Arguments:\n"
        "name: Name of shader. It is mandatory for inline shaders.\n"
        "source: Source code of shader. Use HLSL.\n"
        "mainFunction (optional): entry point of shader. Default is 'main'.\n"
        "Returns: Shader object created.\n"
    ),

    KW_FN(
        get_adapters,
        getAdapters,
        "Lists all gpu graphics cards found in the system.\n"
        "Returns: List of device adapters. Each element in the list is a tuple of type (i : index, name : string)\n"
    ),

    KW_FN(
        get_current_adapter_info,
        getCurrentAdapterInfo,
        "Gets the currently active GPU adapter, as a tuple of (i : index, name : string).\n"
        "Returns: Element as tuple of type (i : index, name : string)\n"
    ),

    KW_FN(
        set_current_adapter,
        setCurrentAdapter,
        "Selects an active GPU adapter. For a list of GPU adapters see coalpy.gpu.get_adapters.\n"
        "Argumnets:\n"
        "index: the desired device index. See coalpy.gpu.get_adapters for a full list.\n"
    ),

    KW_FN(
        schedule,
        schedule,
        ""
    ),


    VA_FN(run, run, "Runs window rendering callbacks. This function blocks until all the existing windows are closed."),

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
    if (!state.checkValidDevice())
        return nullptr;

    const char* shaderName = nullptr;
    const char* shaderSource = nullptr;
    const char* mainFunction = "main";

    static char* argnames[] = { "name", "source", "mainFunction", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "ss|s", argnames, &shaderName, &shaderSource, &mainFunction))
        return nullptr;

    PyObject* obj = PyType_GenericAlloc(state.getType(TypeId::Shader), 1); 
    Shader* shaderObj = reinterpret_cast<Shader*>(obj);
    shaderObj->handle = ShaderHandle();

    ShaderInlineDesc desc;
    desc.type = ShaderType::Compute;
    desc.mainFn = mainFunction;
    desc.name = shaderName;
    desc.immCode = shaderSource;
    shaderObj->db = &state.db();
    shaderObj->handle = state.db().requestCompile(desc);
    return obj;
}

PyObject* getAdapters(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    std::vector<render::DeviceInfo> infos;
    render::IDevice::enumerate(render::DevicePlat::Dx12, infos);
    
    PyObject* newList = PyList_New(0);
    for (auto inf : infos)
    {
        if (!inf.valid)
            continue;

        PyObject* tuple = Py_BuildValue("(is)", inf.index, inf.name.c_str());
        PyList_Append(newList, tuple);
        Py_DECREF(tuple);
    }
    return newList;
}

PyObject* getCurrentAdapterInfo(PyObject* self, PyObject* args, PyObject* kwds)
{
    ModuleState& state = getState(self);
    auto& info = state.device().info();
    if (info.valid)
        return Py_BuildValue("(is)", info.index, info.name.c_str());

    PyErr_SetString(state.exObj(), "coalpy.gpu.get_current_adapter_info failed, current device is invalid.");
    return nullptr;
}

PyObject* setCurrentAdapter(PyObject* self, PyObject* args, PyObject* kwds)
{
    static char* argnames[] = { "index", nullptr };
    int selected = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", argnames, &selected))
        return nullptr;

    ModuleState& state = getState(self);
    if (!state.selectAdapter(selected))
        return nullptr;
    
    Py_RETURN_NONE;
}

PyObject* run(PyObject* self, PyObject* args)
{
    ModuleState& state = getState(self);

    //prepare arguments for this run call.
    RenderArgs* renderArgs = state.alloc<RenderArgs>();
    renderArgs->renderTime = 0.0;
    renderArgs->deltaTime = 0.0;
    
    WindowRunArgs runArgs = {};
    bool raiseException = false;

    Stopwatch stopwatch;

    runArgs.onRender = [&state, &raiseException, renderArgs, &stopwatch]()
    {
        unsigned long long mst = stopwatch.timeMicroSecondsLong();
        double newRenderTime = (double)mst / 1000.0;
        renderArgs->deltaTime = newRenderTime - renderArgs->renderTime;
        renderArgs->renderTime = newRenderTime;

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

    runArgs.listener = &state.windowListener();

    stopwatch.start();
    IWindow::run(runArgs); //block

    CPY_ASSERT(((PyObject*)renderArgs)->ob_refcnt == 1);
    if (raiseException)
    {
        Py_DECREF(renderArgs);
        return nullptr; //means we propagate an exception.
    }

    Py_DECREF(renderArgs);
    Py_RETURN_NONE;
}

PyObject* schedule(PyObject* self, PyObject* args)
{
    Py_RETURN_NONE;
}

}
}
}
