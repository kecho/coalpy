#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <set>
#include <coalpy.render/ShaderDefs.h>
#include <vector>
#include <mutex>

namespace coalpy
{

class IShaderDb;
class IFileSystem;
class IWindowListener;
class ITaskSystem;
class IFileWatcher;

namespace render
{
    class IDevice;
    class CommandList;
}

namespace gpu
{

struct Window;
struct CoalpyTypeObject;

class ModuleState
{
public:
    ModuleState(CoalpyTypeObject** types, int typesCount);
    ~ModuleState();

    void startServices();
    void stopServices();

    IShaderDb&      db() const { return *m_db; }
    IFileSystem&    fs() const { return *m_fs; }
    ITaskSystem&    ts() const { return *m_ts; }
    IWindowListener& windowListener() const { return *m_windowListener; }
    render::IDevice& device() const { return *m_device; }

    void setExObj(PyObject* obj) { m_exObj = obj; }
    PyObject* exObj() const { return m_exObj; }

    void getWindows(std::set<Window*>& outSet) { outSet = m_windows; }
    void registerWindow(Window* w) { m_windows.insert(w); }
    void unregisterWindow(Window* w) { m_windows.erase(w); }
    inline CoalpyTypeObject* getCoalpyType(TypeId t) { return m_types[(int)t]; }
    inline PyTypeObject* getType(TypeId t) { return reinterpret_cast<PyTypeObject*>(m_types[(int)t]); }

    template<typename ObjectT>
    inline ObjectT* alloc(int count = 1)
    {
        return reinterpret_cast<ObjectT*>(PyType_GenericAlloc(getType(ObjectT::s_typeId), count));
    }

    bool selectAdapter(int index);

    bool checkValidDevice();

    static void clean();

    render::CommandList* newCommandList();
    void deleteCommandList(render::CommandList* cmdList);

    void setRenderLoop(bool rl) { m_runningRenderLoop = rl; }
    bool isInRenderLoop() const { return m_runningRenderLoop; }
private:
    void onShaderCompileError(ShaderHandle handle, const char* shaderName, const char* shaderErrorString);
    void registerTypes(CoalpyTypeObject** types, int typesCount);

    bool m_runningRenderLoop = false;

    ITaskSystem*    m_ts;
    IFileSystem*    m_fs;
    IShaderDb*      m_db;
    IFileWatcher*   m_fw;
    render::IDevice* m_device;
    IWindowListener* m_windowListener;
    PyObject* m_exObj;
    CoalpyTypeObject* m_types[(int)TypeId::Counts];
    std::set<Window*> m_windows;

    std::vector<render::CommandList*> m_commandListPool;

    std::mutex m_shaderErrorMutex;

    static std::set<ModuleState*> s_allModules;
};

}

}
