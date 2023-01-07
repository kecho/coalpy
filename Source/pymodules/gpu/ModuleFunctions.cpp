#include "ModuleFunctions.h"
#include "ModuleState.h"
#include "HelperMacros.h"
#include "CommandList.h"
#include <string>
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "ImguiBuilder.h"
#include "ImplotBuilder.h"
#include "Resources.h"
#include "ModuleSettings.h"
#include <coalpy.core/Assert.h>
#include <coalpy.window/IWindow.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.core/Stopwatch.h>
#include <coalpy.core/String.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.texture/ITextureLoader.h>
#include <iostream>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <moduleobject.h>
#include <dictobject.h>

namespace
{

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

#include "bindings/MethodDecl.h"
#include "bindings/ModuleFunctions.inl"

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

void freeModule(void* modulePtr)
{
    PyObject* moduleObj = (PyObject*)modulePtr;
    auto state = (ModuleState*)PyModule_GetState(moduleObj);
    if (state)
        state->~ModuleState();
}


}

static PyMethodDef g_moduleFunctions[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/ModuleFunctions.inl"
    FN_END
};

namespace methods
{

PyMethodDef* get()
{
    return g_moduleFunctions;
}

PyObject* getSettings(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    PyObject* ret = PyDict_New();
    ModuleState& state = getState(self);
    state.settings().dumpToDictionary(ret);
    return ret;
}

PyObject* getAdapters(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    std::vector<render::DeviceInfo> infos;

#if defined(_WIN32)
    auto platform = render::DevicePlat::Dx12;
#elif defined(__linux__)
    auto platform = render::DevicePlat::Vulkan;
#elif
    #error "Platform not supported";
#endif
    render::IDevice::enumerate(platform, infos);
    
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
    static char* argnames[] = { "index", "flags", "dump_shader_pdbs", "shader_model", nullptr };
    int selected = -1;
    int flags = (int)render::DeviceFlags::None;
    int dumpPDBs = 0;
    ShaderModel shaderModel = ShaderModel::Sm6_5;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i|ipi", argnames, &selected, &flags, &dumpPDBs, &shaderModel))
        return nullptr;

    ModuleState& state = getState(self);

    if ((int)shaderModel < (int)ShaderModel::Begin || (int)shaderModel > (int)ShaderModel::End)
    {
        PyErr_SetString(state.exObj(), "Shader model must be a valid value defined in coalpy.gpu.ShaderModel enums");
        return nullptr;
    }

    if (!state.selectAdapter(selected, flags, shaderModel, (bool)dumpPDBs))
        return nullptr;
    
    Py_RETURN_NONE;
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
    renderArgs->implotBuilder = Py_None;
    Py_INCREF(Py_None);

    ImguiBuilder* imguiBuilder = state.alloc<ImguiBuilder>();
    new (imguiBuilder) ImguiBuilder;
    ImplotBuilder* implotBuilder = state.alloc<ImplotBuilder>();
    new (implotBuilder) ImplotBuilder;
    
    WindowRunArgs runArgs = {};
    bool raiseException = false;

    Stopwatch stopwatch;

    auto realoadUITexturesCb = [&state](render::Texture texture)
    {
        std::set<Window*> windowsPtrs;
        state.getWindows(windowsPtrs);
        for (Window* w : windowsPtrs)
        {
            if (w->uiRenderer == nullptr)
                continue;

            if (!w->uiRenderer->isTextureRegistered(texture))
                continue;
        
            w->uiRenderer->unregisterTexture(texture);
            w->uiRenderer->registerTexture(texture); //re-register
        }
    };

    runArgs.onRender = [&realoadUITexturesCb, &state, &raiseException, renderArgs, &stopwatch, imguiBuilder, implotBuilder]()
    {
        state.tl().processTextures(realoadUITexturesCb);

        unsigned long long mst = stopwatch.timeMicroSecondsLong();
        double newRenderTime = (double)mst / 1000.0;
        renderArgs->deltaTime = newRenderTime - renderArgs->renderTime;
        renderArgs->renderTime = newRenderTime;

        std::set<Window*> windowsPtrs;
        state.getWindows(windowsPtrs);
        int openedWindows = 0;
        int renderedWindows = 0;
        for (Window* w : windowsPtrs)
        {
            CPY_ASSERT(w != nullptr);
            if (w == nullptr)
                return false;

            if (w->object->isClosed())
                continue;
            ++openedWindows;
            
            if (!w->object->shouldRender())
                continue;
            ++renderedWindows;

            if (w->uiRenderer != nullptr)
            {
                renderArgs->imguiBuilder = (PyObject*)imguiBuilder;
                imguiBuilder->enabled = true;
                imguiBuilder->activeRenderer = w->uiRenderer;

                renderArgs->implotBuilder = (PyObject*)implotBuilder;
                implotBuilder->enabled = true;
                implotBuilder->activeRenderer = w->uiRenderer;
                w->uiRenderer->newFrame();
                state.setTextureDestructionCallback([&windowsPtrs](Texture& t) { unregisterAllImguiTextures(windowsPtrs, t); } );
            }

            renderArgs->window = w;
            Py_INCREF(w);
            renderArgs->userData = w->userData;
            Py_INCREF(w->userData);
            w->object->dimensions(renderArgs->width, renderArgs->height);
            
            if (w && w->onRenderCallback != nullptr)
            {
                Py_XINCREF(w->onRenderCallback);
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

            if (w->display != nullptr)
                w->display->present();

            Py_DECREF(renderArgs->window);
            renderArgs->window = nullptr;
            Py_DECREF(renderArgs->userData);
            renderArgs->userData = nullptr;
            renderArgs->imguiBuilder = Py_None;
            renderArgs->implotBuilder = Py_None;
            imguiBuilder->enabled = false;
            imguiBuilder->activeRenderer = nullptr;
            imguiBuilder->clearTextureReferences();
            state.setTextureDestructionCallback(nullptr);

            implotBuilder->enabled = false;
            implotBuilder->activeRenderer = nullptr;
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

PyObject* beginCollectMarkers(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = getState(self);
    if (!moduleState.checkValidDevice())
    {
        PyErr_SetString(moduleState.exObj(), "Cant schedule, current device is invalid.");
        return nullptr;
    }

    int maxQueryBytes = 56 * 1024;
    char* arguments[] = { "max_query_bytes", nullptr };
    PyObject* cmdListsArg = nullptr;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|i", arguments, &maxQueryBytes))
        return nullptr;

    moduleState.device().beginCollectMarkers(maxQueryBytes);
    Py_RETURN_NONE;
}

PyObject* endCollectMarkers(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = getState(self);
    if (!moduleState.checkValidDevice())
    {
        PyErr_SetString(moduleState.exObj(), "Cant schedule, current device is invalid.");
        return nullptr;
    }

    render::MarkerResults results = moduleState.device().endCollectMarkers();
    if (!results.timestampBuffer.success())
    {
        PyErr_SetString(moduleState.exObj(), "Failed to extract gpu results for markers.");
        return nullptr;
    }

    auto* markerResultObj = moduleState.alloc<MarkerResults>();
    new (markerResultObj) MarkerResults;

    {
        auto* bufferResult = moduleState.alloc<Buffer>();
        new (bufferResult) Buffer;
        bufferResult->owned = false;
        bufferResult->buffer = results.timestampBuffer;
        markerResultObj->timestampBuffer = bufferResult;
    }

    markerResultObj->markers = PyList_New(results.markerCount);
    for (int i = 0; i < results.markerCount; ++i)
    {
        const render::MarkerTimestamp& m = results.markers[i];
        PyList_SetItem(
            markerResultObj->markers, i,
            Py_BuildValue("(siii)", m.name.c_str(), m.parentMarkerIndex, m.beginTimestampIndex, m.endTimestampIndex));
    }
    
    markerResultObj->timestampFrequency = results.timestampFrequency;
    
    return (PyObject*)markerResultObj;
}

}
}
}
