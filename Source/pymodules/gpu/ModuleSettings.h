#pragma once

#include "SettingsSchema.h"

namespace coalpy
{
namespace gpu
{

struct ModuleSettings
{
    BEGIN_PARAM_TABLE(ModuleSettings)
        REGISTER_PARAM(adapter_index)
        REGISTER_PARAM(adapter_preference)
        REGISTER_PARAM(graphics_api)
    END_PARAM_TABLE()

    int adapter_index = 0;
    std::string adapter_preference = "auto";
    std::string graphics_api = "dx12";
};

}
}
