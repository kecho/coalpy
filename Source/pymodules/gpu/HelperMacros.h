#pragma once

//Used for python function declarations
#define KW_FN(pyname, name, desc) \
    { #pyname, (PyCFunction)(coalpy::gpu::methods::name), METH_VARARGS | METH_KEYWORDS, desc }

#define VA_FN(pyname, name, desc) \
    { #pyname, (coalpy::gpu::methods::name), METH_VARARGS, desc }

#define FN_END \
    {NULL, NULL, 0, NULL}

//Used to register types
#define RegisterType(ctypeName, list)\
    {\
        CoalpyTypeObject* to = new CoalpyTypeObject;\
        to->pyObj = { PyVarObject_HEAD_INIT(NULL, 0) };\
        ctypeName::constructType(to->pyObj);\
        to->typeId = ctypeName::s_typeId;\
        list.push_back(to);\
    }

