#include "ModuleFunctions.h"
#include "ModuleState.h"
#include "HelperMacros.h"
#include "CommandList.h"
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
    if (state.isInRenderLoop())
    {
        PyErr_SetString(state.exObj(), "Cannot call coalpy.gpu.run() method inside itself.");
        return nullptr;
    }

    //prepare arguments for this run call.
    RenderArgs* renderArgs = state.alloc<RenderArgs>();
    renderArgs->window = nullptr;
    renderArgs->userData = nullptr;
    renderArgs->renderTime = 0.0;
    renderArgs->deltaTime = 0.0;
    renderArgs->width = 0;
    renderArgs->height = 0;
    
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

            renderArgs->window = w;
            Py_INCREF(w);
            renderArgs->userData = w->userData;
            Py_INCREF(w->userData);

            w->object->dimensions(renderArgs->width, renderArgs->height);
            ++openedWindows;
            if (w && w->onRenderCallback != nullptr)
            {
                PyObject* retObj = PyObject_CallOneArg(w->onRenderCallback, (PyObject*)renderArgs);
                //means an exception has been risen. Propagate up.
                if (retObj == nullptr)
                {
                    Py_DECREF(renderArgs->window);
                    renderArgs->window = nullptr;
                    Py_DECREF(renderArgs->userData);
                    renderArgs->userData = nullptr;
                    raiseException = true;
                    return false;
                }
                Py_DECREF(retObj);
            }

            w->display->present();
            Py_DECREF(renderArgs->window);
            renderArgs->window = nullptr;
            Py_DECREF(renderArgs->userData);
            renderArgs->userData = nullptr;
        }

        return openedWindows != 0;
    };

    runArgs.listener = &state.windowListener();

    stopwatch.start();
    state.setRenderLoop(true);
    IWindow::run(runArgs); //block
    state.setRenderLoop(false);

    if (raiseException)
    {
        Py_DECREF(renderArgs);
        return nullptr; //means we propagate an exception.
    }

    Py_DECREF(renderArgs);
    Py_RETURN_NONE;
}

PyObject* schedule(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = getState(self);
    if (!moduleState.checkValidDevice())
    {
        PyErr_SetString(moduleState.exObj(), "Cant schedule, current device is invalid.");
        return nullptr;
    }

    char* arguments[] = { "command_lists", nullptr };
    PyObject* cmdListsArg = nullptr;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "O", arguments, &cmdListsArg))
            return nullptr;

    if (!PyList_Check(cmdListsArg) || Py_SIZE(cmdListsArg) == 0)
    {
        PyErr_SetString(moduleState.exObj(), "argument command lists must be a non empty list");
        return nullptr;
    }

    int commandListsCounts = Py_SIZE(cmdListsArg);
    auto& listObj = *((PyListObject*)cmdListsArg);
    std::vector<render::CommandList*> cmdListsVector;
    PyTypeObject* pyCmdListType = moduleState.getType(CommandList::s_typeId);
    for (int i = 0; i < commandListsCounts; ++i)
    {
        PyObject* obj = listObj.ob_item[i];
        if (obj->ob_type != pyCmdListType)
        {
            PyErr_SetString(moduleState.exObj(), "object inside command list argument list must be of type CommandList.");
            return nullptr;
        }

        CommandList& cmdListObj = *((CommandList*)obj);
        cmdListObj.cmdList->finalize();
        cmdListsVector.push_back(cmdListObj.cmdList);
    }

    auto result = moduleState.device().schedule(cmdListsVector.data(), (int)cmdListsVector.size());
    if (!result.success())
    {
        PyErr_Format(moduleState.exObj(), "schedule call failed, reson: %s", result.message.c_str());
        return nullptr;
    }

    Py_RETURN_NONE;
}

}
}
}
