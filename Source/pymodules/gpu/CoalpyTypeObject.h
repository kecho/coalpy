#pragma once

#include "ModuleState.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <vector>

namespace coalpy
{
namespace gpu
{

class ModuleState;

struct EnumEntry
{
    const char* name;
    int value;
    const char* docs;
};

struct ChildTypeObject
{
    const char* name;
    PyTypeObject* pyObj;
};

struct CoalpyTypeObject
{
    //Must be the first member, offset 0
    PyTypeObject pyObj = { PyVarObject_HEAD_INIT(NULL, 0) };
    ModuleState* moduleState = nullptr;
    std::vector<ChildTypeObject> children;
    TypeId typeId;
}; 

struct CoalpyGenericEnumTypeObject : CoalpyTypeObject
{
    const char* objectName;
    const EnumEntry* entries;
    std::vector<PyMemberDef> memberDefs;
    int entriesCount;
};

static_assert(offsetof(CoalpyTypeObject, pyObj) == 0);

inline CoalpyTypeObject* asCoalpyObj(PyTypeObject* obj)
{
    return reinterpret_cast<CoalpyTypeObject*>(obj);
}

inline ModuleState& parentModule(PyObject* object)
{
    return *(asCoalpyObj(Py_TYPE(object))->moduleState);
}

}
}
