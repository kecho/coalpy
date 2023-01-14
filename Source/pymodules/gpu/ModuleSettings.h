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
        REGISTER_PARAM(enable_debug_device, "Enables debug device settings for dx12 or vulkan (verbose device warnings).")
        REGISTER_PARAM(dump_shader_pdbs, "Dumps shader pdbs for debugging. Only works on dx12.")
        REGISTER_PARAM(adapter_index, "Current adapter index to use.")
        REGISTER_PARAM(graphics_api, "Graphics api to use. Valid strings are \"dx12\" or \"vulkan\" case sensitive.")
        REGISTER_PARAM(shader_model, "HLSL shader model to use. Can be sm6_0, sm6_1, sm6_2, sm6_3, sm6_4, sm6_5. The system will try and find the maximum possible")
    END_PARAM_TABLE()

    static const char* sSettingsFileName;

    //Data
    PyObject_HEAD
    bool enable_debug_device = false;
    bool dump_shader_pdbs = false;
    int adapter_index = 0;
    std::string graphics_api = "default";
    std::string shader_model = "sm6_5";

    //Functions
    static const TypeId s_typeId = TypeId::ModuleSettings;
    static void constructType(CoalpyTypeObject& t);
};

}
}
