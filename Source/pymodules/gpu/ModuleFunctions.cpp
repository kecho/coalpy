#include "ModuleFunctions.h"
#include "ModuleState.h"
#include "HelperMacros.h"
#include "CommandList.h"
#include <string>
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "ImguiBuilder.h"
#include "Resources.h"
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.core/Stopwatch.h>
#include <coalpy.core/String.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.texture/ITextureLoader.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <moduleobject.h>

namespace {

PyMethodDef g_defs[] = {

    KW_FN(
        get_adapters,
        getAdapters,
        R"(
        Lists all gpu graphics cards found in the system.

        Returns:
            adapters (Array of tuple): Each element in the list is a tuple of type (i : int, name : Str)
        )"
    ),

    KW_FN(
        get_current_adapter_info,
        getCurrentAdapterInfo,
        R"(
        Gets the currently active GPU adapter, as a tuple of (i : int, name : Str).

        Returns:
            adapter (tuple): Element as tuple of type (i : int, name : str)
        )"
    ),

    KW_FN(
        set_current_adapter,
        setCurrentAdapter,
        R"(
        Selects an active GPU adapter. For a list of GPU adapters see coalpy.gpu.get_adapters.

        Parameters:
            index (int): the desired device index. See get_adapters for a full list.
            flags (gpu.DeviceFlags) (optional): bit mask with device flags.
        )"
    ),

    KW_FN(
        add_data_path,
        addDataPath,
        R"(
        Adds a path to load shader / texture files / shader includes from.
        
        Parameters:
            path (str or Module):  If str it will be a path to be used. If a module, it will find the source folder of the module's __init__.py file, and use this source as the path.
                                   path priorities are based on the order of how they were added.
        )"
    ),

    KW_FN(
        schedule,
        schedule,
        R"(
        Submits an array of CommandList objects to the GPU to run shader work on.

        Parameters:
            command_lists (array of CommandList or a single CommandList object): an array of CommandList objects or a single CommandList object to run in the GPU. CommandList can be resubmitted through more calls of schedule.
        )"
    ),

    VA_FN(run, run, "Runs window rendering callbacks. This function blocks until all the existing windows are closed. Window objects must be created and referenced prior. Use the Window object to configure / specify callbacks and this function to run all the event loops for windows."),

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
    static char* argnames[] = { "index", "flags", nullptr };
    int selected = -1;
    int flags = (int)render::DeviceFlags::None;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i|i", argnames, &selected, &flags))
        return nullptr;

    ModuleState& state = getState(self);
    if (!state.selectAdapter(selected, flags))
        return nullptr;
    
    Py_RETURN_NONE;
}

bool getModulePath(ModuleState& moduleState, PyObject* moduleObject, std::string& path)
{
    const bool isModule = PyModule_Check(moduleObject);
    CPY_ASSERT(isModule);
    if (!isModule)
        return false;

    {
        PyObject* filenameObj = PyModule_GetFilenameObject(moduleObject);
        if (filenameObj == nullptr)
        {
            PyErr_SetString(moduleState.exObj(), "Module passed does not contain an __init__.py file or a core location that can be used to extract a path.");
            return false;
        }

        if (PyUnicode_Check(filenameObj))
        {
            std::wstring wstr = (const wchar_t*)PyUnicode_AS_DATA(filenameObj);
            std::string coreModulePath = ws2s(wstr);
            FileUtils::getDirName(coreModulePath, path);
        }

        Py_DECREF(filenameObj);
    }

    return true;

}

PyObject* addDataPath(PyObject* self, PyObject* args, PyObject* kwds)
{
    ModuleState& moduleState = getState(self);
    if (!moduleState.checkValidDevice())
    {
        PyErr_SetString(moduleState.exObj(), "Can't add a data path, current device is invalid.");
        return nullptr;
    }

    PyObject* object = nullptr;
    static char* argnames[] = { "path", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", argnames, &object))
        return nullptr;

    std::string path;
    if (PyUnicode_Check(object))
    {
        std::wstring wstr = (const wchar_t*)PyUnicode_AS_DATA(object);
        path = ws2s(wstr);
    }
    else if (!getModulePath(moduleState, self, path))
    {
        return nullptr;
    }

    bool exists = false;
    bool isDir = false;
    bool isDots = false;
    FileAttributes attr;
    moduleState.fs().getFileAttributes(path.c_str(), attr);

    if (attr.isDot)
    {
        PyErr_Format(moduleState.exObj(), "Path passed %s is a dot type (. or ..). These are not permitted.", path.c_str());
        return nullptr;
    }

    if (!attr.exists)
    {
        PyErr_Format(moduleState.exObj(), "Path passed doesn't exist: %s", path.c_str());
        return nullptr;
    }

    if (!attr.isDir)
    {
        PyErr_Format(moduleState.exObj(), "Path passed must be a directory: %s", path.c_str());
        return nullptr;
    }

    moduleState.addDataPath(path.c_str());

    Py_RETURN_NONE;

}

void unregisterAllImguiTextures(const std::set<Window*>& windowsPtrs, Texture& t)
{
    if (!t.hasImguiRef)
        return;

    for (Window* w : windowsPtrs)
    {
        if (w->uiRenderer == nullptr)
            continue;   

        w->uiRenderer->unregisterTexture(t.texture);
    }

    t.hasImguiRef = false;
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
    new (renderArgs) RenderArgs;
    renderArgs->imguiBuilder = Py_None;
    Py_INCREF(Py_None);

    ImguiBuilder* imguiBuilder = state.alloc<ImguiBuilder>();
    new (imguiBuilder) ImguiBuilder;
    
    WindowRunArgs runArgs = {};
    bool raiseException = false;

    Stopwatch stopwatch;

    runArgs.onRender = [&state, &raiseException, renderArgs, &stopwatch, imguiBuilder]()
    {
        state.tl().processTextures();

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

            if (w->uiRenderer != nullptr)
            {
                renderArgs->imguiBuilder = (PyObject*)imguiBuilder;
                imguiBuilder->enabled = true;
                imguiBuilder->activeRenderer = w->uiRenderer;
                w->uiRenderer->newFrame();
                state.setTextureDestructionCallback([&windowsPtrs](Texture& t) { unregisterAllImguiTextures(windowsPtrs, t); } );
            }

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

            if (w->uiRenderer != nullptr)
                w->uiRenderer->render();

            w->display->present();
            Py_DECREF(renderArgs->window);
            renderArgs->window = nullptr;
            Py_DECREF(renderArgs->userData);
            renderArgs->userData = nullptr;
            renderArgs->imguiBuilder = Py_None;
            imguiBuilder->enabled = false;
            imguiBuilder->activeRenderer = nullptr;
            imguiBuilder->clearTextureReferences();
            state.setTextureDestructionCallback(nullptr);
        }

        return openedWindows != 0;
    };

    runArgs.listener = &state.windowListener();

    stopwatch.start();
    state.setRenderLoop(true);
    IWindow::run(runArgs); //block
    state.setRenderLoop(false);

    Py_DECREF(imguiBuilder);
    Py_DECREF(renderArgs);
    Py_DECREF(Py_None);

    if (raiseException)
        return nullptr;

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

    PyTypeObject* pyCmdListType = moduleState.getType(CommandList::s_typeId);
    std::vector<render::CommandList*> cmdListsVector;

    if (PyList_Check(cmdListsArg) && Py_SIZE(cmdListsArg) > 0)
    {
        int commandListsCounts = Py_SIZE(cmdListsArg);
        auto& listObj = *((PyListObject*)cmdListsArg);
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
    }
    else if (cmdListsArg->ob_type == pyCmdListType)
    {
        CommandList& cmdListObj = *((CommandList*)cmdListsArg);
        cmdListObj.cmdList->finalize();
        cmdListsVector.push_back(cmdListObj.cmdList);
    }
    else
    {
        PyErr_SetString(moduleState.exObj(), "argument command lists must be a non empty list of gpu.CommandList or a single CommandList object.");
        return nullptr;
    }

    auto result = moduleState.device().schedule(cmdListsVector.data(), (int)cmdListsVector.size());
    if (!result.success())
    {
        PyErr_Format(moduleState.exObj(), "schedule call failed, reason: %s", result.message.c_str());
        return nullptr;
    }

    Py_RETURN_NONE;
}

}
}
}
