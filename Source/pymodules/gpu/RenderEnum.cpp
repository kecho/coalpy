#include "RenderEnum.h"
#include <structmember.h>
#include <algorithm>
#include <sstream>
#include <stdio.h>

namespace coalpy
{
namespace gpu
{

int genericEnumInit(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    PyErr_SetString(moduleState.exObj(), "Cannot instantiate the meta type of an enumeration. Just use an int to hold a value, and use the declared enum values in the coalpy.gpu module.");
    return -1;
}

CoalpyGenericEnumTypeObject* RenderEnum::constructEnumType(const char* typeName, const char* objectName, const EnumEntry* enums, const char* doc)
{
    const EnumEntry* e = enums;

    auto* ctp = new CoalpyGenericEnumTypeObject;
    ctp->objectName = objectName;
    ctp->typeId = TypeId::GenericEnum;
    ctp->moduleState = nullptr;

    int counts = 0;
    while (enums[counts].name != nullptr)
    {
        auto& entry = enums[counts];
        PyMemberDef def = {
            entry.name,
            T_INT,
            (Py_ssize_t)(offsetof(RenderEnum, values) + sizeof(int)*(Py_ssize_t)counts),
            READONLY,
            entry.docs };
        ctp->memberDefs.push_back(def);
        ++counts;
    }

    {
        PyMemberDef sentry = {};
        ctp->memberDefs.push_back(sentry);
    }

    ctp->entries = enums;
    ctp->entriesCount = counts;

    auto& t = ctp->pyObj;

    t = { PyVarObject_HEAD_INIT(NULL, 0) };
    t.tp_name = typeName;
    t.tp_basicsize = sizeof(RenderEnum) + sizeof(int) * counts;
    t.tp_init = genericEnumInit;
    t.tp_doc   = doc;
    t.tp_members = ctp->memberDefs.data();
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;

    return ctp;
}

void RenderEnum::registerInModule(CoalpyGenericEnumTypeObject& to, PyObject& moduleObject)
{
    PyObject* newObj = PyType_GenericAlloc(&to.pyObj, 1);
    auto enumObj = (RenderEnum*)newObj;
    for (int e = 0; e < to.entriesCount; ++e)
        enumObj->values[e] = to.entries[e].value;

    PyModule_AddObject(&moduleObject, to.objectName, newObj);
}


}
}
