#pragma once

#define RegisterType(type, list)\
    {\
        PyTypeObject to = { PyVarObject_HEAD_INIT(NULL, 0) };\
        type::makeType(to);\
        list.push_back(to);\
    }
