#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace coalpy
{

class IShaderService;
class IShaderDb;
class IFileSystem;
class ITaskSystem;

namespace gpu
{

class ModuleState
{
public:
    ModuleState();
    ~ModuleState();

    void startServices();
    void stopServices();

    IShaderDb&      db() const { return *m_db; }
    IShaderService& ss() const { return *m_ss; }
    IFileSystem&    fs() const { return *m_fs; }
    ITaskSystem&    ts() const { return *m_ts; }

    void setExObj(PyObject* obj) { m_exObj = obj; }
    PyObject* exObj() const { return m_exObj; }

private:
    ITaskSystem*    m_ts;
    IFileSystem*    m_fs;
    IShaderDb*      m_db;
    IShaderService* m_ss;
    PyObject* m_exObj;
};

}

}
