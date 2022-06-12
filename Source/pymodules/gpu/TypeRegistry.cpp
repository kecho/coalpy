#include "TypeRegistry.h"
#include "HelperMacros.h"
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "Resources.h"
#include "RenderEnum.h"
#include "CommandList.h"
#include "ResourceDownloadRequest.h"
#include "CoalpyTypeObject.h"
#include "ImguiBuilder.h"
#include "ImplotBuilder.h"
#include <coalpy.render/IDevice.h>
#include <coalpy.render/ShaderDefs.h>
#include <coalpy.core/Formats.h>
#include <coalpy.core/Assert.h>
#include <coalpy.window/Keys.h>

namespace coalpy
{
namespace gpu
{

CoalpyTypeObject* registerFormats()
{
    static std::vector<EnumEntry> s_formats;
    if (s_formats.empty())
    {
        s_formats.resize((int)Format::MAX_COUNT);
        for (int f = 0; f < (int)s_formats.size(); ++f)
        {
            auto& e = s_formats[f];
            e.name = getFormatName((Format)f);
            e.value = f;
            e.docs = "";
        }
        //terminator
        s_formats.emplace_back();
        s_formats.back() = {};
    }

    return RenderEnum::constructEnumType("gpu.EnumFormat", "Format", s_formats.data(), "Formats supported for Textures and Standard Buffers.");
}


CoalpyTypeObject* registerKeys()
{
    static const EnumEntry s_InputKeys[] = {
        #define KEY_DECL(x) { #x, (int)Keys::x, #x " key enumeration." },
        #include <coalpy.window/Keys.inl>
        #undef KEY_DECL
        { nullptr, 0, nullptr }
    };
    return RenderEnum::constructEnumType("gpu.EnumKeys", "Keys",  s_InputKeys, "Enumeration of keyboard / mouse input keys. To be used with the input_state object from the Window. Use enum values located at coalpy.gpu.Keys"); 
}


void constructTypes(TypeList& outTypes)
{
    //** Register Types **//
    RegisterType(Window,                  outTypes);
    RegisterType(Shader,                  outTypes);
    RegisterType(RenderArgs,              outTypes);
    RegisterType(Sampler,                 outTypes);
    RegisterType(Buffer,                  outTypes);
    RegisterType(CommandList,             outTypes);
    RegisterType(Texture,                 outTypes);
    RegisterType(InResourceTable,         outTypes);
    RegisterType(OutResourceTable,        outTypes);
    RegisterType(ResourceDownloadRequest, outTypes);
    RegisterType(SamplerTable,            outTypes);
    RegisterType(ImguiBuilder,            outTypes);
    RegisterType(ImplotBuilder,           outTypes);
    
    //** Register Enums **//
    {
        #include "bindings/EnumDef.h"
        #include "bindings/Enums.inl"
    }

    {
        #include "bindings/EnumDef.h"
        #include "bindings/Imgui.inl"
    }

    {
        #include "bindings/EnumDef.h"
        #include "bindings/Implot.inl"
    }

    outTypes.push_back(registerKeys());
    outTypes.push_back(registerFormats());
}

void processTypes(TypeList& types, ModuleState* moduleState, PyObject& moduleObject)
{
    for (auto t : types)
    {
        if (t->typeId == TypeId::GenericEnum)
        {
            t->moduleState = moduleState;
            RenderEnum::registerInModule((CoalpyGenericEnumTypeObject&)(*t), moduleObject);
        }
    }
}

}
}
