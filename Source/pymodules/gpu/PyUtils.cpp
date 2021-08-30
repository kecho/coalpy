#include "PyUtils.h"
#include "ModuleState.h"


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

}
}
