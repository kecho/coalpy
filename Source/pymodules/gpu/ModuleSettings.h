#pragma once

#include "SettingsSchema.h"
#include "TypeIds.h"
#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace coalpy
{
namespace gpu
{

struct CoalpyTypeObject;

struct ModuleSettings
{
    BEGIN_PARAM_TABLE(ModuleSettings)
        REGISTER_PARAM(adapter_index, "Current adapter index to use.")
        REGISTER_PARAM(graphics_api, "Graphics api to use. Valid strings are \"dx12\" or \"vulkan\" case sensitive.")
    END_PARAM_TABLE()

    static const char* sSettingsFileName;

    //Data
    PyObject_HEAD
    int adapter_index = 0;
    std::string graphics_api = "default";

    //Functions
    static const TypeId s_typeId = TypeId::ModuleSettings;
    static void constructType(CoalpyTypeObject& t);
};

}
}
