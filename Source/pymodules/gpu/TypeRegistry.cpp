#include "TypeRegistry.h"
#include "HelperMacros.h"
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "Resources.h"
#include "RenderEnum.h"
#include "CoalpyTypeObject.h"
#include <coalpy.core/Formats.h>
#include <coalpy.core/Assert.h>

#define EnumsBegin(obj)\
{\
    auto& __o = obj;

#define RegisterEnum(Name, Value)\
    PyModule_AddIntConstant(&__o, #Name, (int)Value)

#define EnumsEnd()\
}

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

    return RenderEnum::constructEnumType("gpu.EnumFormat", "Format", s_formats.data(), "");
}

void constructTypes(TypeList& outTypes)
{
    //** Register Types **//
    RegisterType(Window,             outTypes);
    RegisterType(Shader,             outTypes);
    RegisterType(RenderArgs,         outTypes);
    RegisterType(Buffer,             outTypes);
    RegisterType(Texture,            outTypes);
    RegisterType(InResourceTable,    outTypes);
    RegisterType(OutResourceTable,   outTypes);
    
    //** Register Enums **//
    {
        static EnumEntry s_TextureType[] = {
            { "k1d",          (int)render::TextureType::k1d, "" },
            { "k2d",          (int)render::TextureType::k2d, "" },
            { "k2dArray",     (int)render::TextureType::k2dArray, "" },
            { "k3d",          (int)render::TextureType::k3d, "" },
            { "CubeMap",      (int)render::TextureType::CubeMap, "" },
            { "CubeMapArray", (int)render::TextureType::CubeMapArray, "" },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumTextureType", "TextureType", s_TextureType, "")); 
    }

    {
        static EnumEntry s_BufferType[] = {
            { "Standard",   (int)render::BufferType::Standard, "" },
            { "Structured", (int)render::BufferType::Structured, "" },
            { "Raw",        (int)render::BufferType::Raw, "" },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumBufferType", "BufferType", s_BufferType, "")); 
    }

    {
        static EnumEntry s_MemFlags[] = {
            { "CpuRead",  render::MemFlag_CpuRead, "" },
            { "CpuWrite", render::MemFlag_CpuWrite, "" },
            { "GpuRead",  render::MemFlag_GpuRead, "" },
            { "GpuWrite", render::MemFlag_GpuWrite, "" },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumMemFlags", "MemFlags", s_MemFlags, "")); 
    }

    outTypes.push_back(registerFormats());
}

void processTypes(TypeList& types, PyObject& moduleObject)
{
    for (auto t : types)
    {
        if (t->typeId == TypeId::GenericEnum)
            RenderEnum::registerInModule((CoalpyGenericEnumTypeObject&)(*t), moduleObject);
    }
}

}
}
