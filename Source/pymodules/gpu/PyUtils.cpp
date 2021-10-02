#include "PyUtils.h"
#include "ModuleState.h"
#include <tupleobject.h>
#include <listobject.h>
#include <longobject.h>

namespace coalpy
{
namespace gpu
{

bool getArrayOfNums(ModuleState& moduleState, PyObject* constants, std::vector<int>& rawNums, bool allowFloat, bool allowInt)
{
    if (!PyList_Check(constants))
        return false;

    auto& listObj = *((PyListObject*)constants);
    int listSize = Py_SIZE(constants);
    rawNums.reserve(listSize);
    for (int i = 0; i < listSize; ++i)
    {
        PyObject* obj = listObj.ob_item[i];
        bool isLong = PyLong_Check(obj);
        if (PyLong_Check(obj))
        {
            if (allowInt)
                rawNums.push_back((int)PyLong_AsLong(obj));
            else
                return false;
        }
        else if (PyFloat_Check(obj))
        {
            if (allowFloat)
            {
                float f = (float)(PyFloat_AS_DOUBLE(obj));
                rawNums.push_back(*reinterpret_cast<int*>((&f)));
            }
            else
                return false;
        }
        else
            return false;
    }

    return true;
}

bool getTupleValues(PyObject* obj, int* outArray, int minCount, int maxCount)
{
    if (obj == nullptr)
        return false;

    if (PyTuple_Check(obj))
    {
        int sz = (int)PyTuple_Size(obj);
        if (sz < minCount || sz > maxCount)
            return false;

        for (int i = 0; i < sz; ++i)
        {
            PyObject* item = PyTuple_GetItem(obj, i);
            if (item == nullptr || !PyLong_Check(item))
                return false;

            outArray[i] = (int)PyLong_AsLong(item);
        }

        return true;
    }
    else if (PyList_Check(obj))
    {
        int sz = (int)PyList_Size(obj);
        if (sz < minCount || sz > maxCount)
            return false;

        for (int i = 0; i < sz; ++i)
        {
            PyObject* item = PyList_GetItem(obj, i);
            if (item == nullptr || !PyLong_Check(item))
                return false;

            outArray[i] = (int)PyLong_AsLong(item);
        }
        return true;
    }
    else
        return false;
}

}
}
