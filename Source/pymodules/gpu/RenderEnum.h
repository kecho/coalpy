#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "TypeIds.h"
#include "CoalpyTypeObject.h"

namespace coalpy
{

namespace gpu
{

struct RenderEnum
{
    //Data
    PyObject_HEAD

    //must be at the end.
    int values[0];

    static CoalpyGenericEnumTypeObject* constructEnumType(const char* typeName, const char* objectName, const EnumEntry* enums, const char* doc);
    static void registerInModule(CoalpyGenericEnumTypeObject& to, PyObject& moduleObject);
};

}

}
