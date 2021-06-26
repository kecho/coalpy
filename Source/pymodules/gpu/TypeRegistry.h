#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <vector>

namespace coalpy
{

namespace gpu
{

struct CoalpyTypeObject;
class ModuleState;

using TypeList = std::vector<CoalpyTypeObject*>;
void constructTypes(TypeList& outTypes);
void processTypes(TypeList& typeList, PyObject& moduleObject);

}
}
