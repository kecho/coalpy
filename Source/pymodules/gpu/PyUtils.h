#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <vector>

namespace coalpy
{
namespace gpu
{

class ModuleState;

bool getArrayOfNums(
    ModuleState& moduleState,
    PyObject* array,
    std::vector<int>& rawNums, bool allowFloat = true, bool allowInt = true);

bool getTupleValues(PyObject* obj, int* outArray, int minCount, int maxCount);

}
}
