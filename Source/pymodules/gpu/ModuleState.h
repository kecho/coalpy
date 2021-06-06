#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"

namespace coalpy
{

class IShaderService;
class IShaderDb;
class IFileSystem;
class ITaskSystem;

namespace gpu
{

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

private:
    void registerTypes(CoalpyTypeObject** types, int typesCount);
    ITaskSystem*    m_ts;
    IFileSystem*    m_fs;
    IShaderDb*      m_db;
    IShaderService* m_ss;
    PyObject* m_exObj;
    CoalpyTypeObject* m_types[(int)TypeId::Counts];
};

}

}
