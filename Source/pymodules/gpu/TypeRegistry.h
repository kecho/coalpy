#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <vector>

namespace coalpy
{

namespace gpu
{

struct CoalpyTypeObject;

using TypeList = std::vector<CoalpyTypeObject*>;
//registration table
void constructTypes(TypeList& outTypes);

}
}
