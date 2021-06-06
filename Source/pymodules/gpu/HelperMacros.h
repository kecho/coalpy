#pragma once

#define RegisterType(ctypeName, list)\
    {\
        CoalpyTypeObject* to = new CoalpyTypeObject;\
        to->pyObj = { PyVarObject_HEAD_INIT(NULL, 0) };\
        ctypeName::constructType(to->pyObj);\
        to->typeId = ctypeName::s_typeId;\
        list.push_back(to);\
    }
