#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include <set>
#include <coalpy.render/ShaderDefs.h>
#include <mutex>

namespace coalpy
{

class IShaderService;
class IShaderDb;
class IFileSystem;
class ITaskSystem;

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
    IShaderService& ss() const { return *m_ss; }
    IFileSystem&    fs() const { return *m_fs; }
    ITaskSystem&    ts() const { return *m_ts; }

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

private:
    void onShaderCompileError(ShaderHandle handle, const char* shaderName, const char* shaderErrorString);
    void registerTypes(CoalpyTypeObject** types, int typesCount);

    ITaskSystem*    m_ts;
    IFileSystem*    m_fs;
    IShaderDb*      m_db;
    IShaderService* m_ss;
    PyObject* m_exObj;
    CoalpyTypeObject* m_types[(int)TypeId::Counts];
    std::set<Window*> m_windows;

    std::mutex m_shaderErrorMutex;
};

}

}
