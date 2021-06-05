#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <vector>

namespace coalpy
{

namespace gpu
{

//registration table
void constructTypes(std::vector<PyTypeObject>& outTypes);

}
}
