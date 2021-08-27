#include "TypeRegistry.h"
#include "HelperMacros.h"
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "Resources.h"
#include "RenderEnum.h"
#include "CommandList.h"
#include "CoalpyTypeObject.h"
#include "ImguiBuilder.h"
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

    return RenderEnum::constructEnumType("gpu.EnumFormat", "Format", s_formats.data(), "Formats supported for Textures and Standard Buffers.");
}

void constructTypes(TypeList& outTypes)
{
    //** Register Types **//
    RegisterType(Window,             outTypes);
    RegisterType(Shader,             outTypes);
    RegisterType(RenderArgs,         outTypes);
    RegisterType(Sampler,            outTypes);
    RegisterType(Buffer,             outTypes);
    RegisterType(CommandList,        outTypes);
    RegisterType(Texture,            outTypes);
    RegisterType(InResourceTable,    outTypes);
    RegisterType(OutResourceTable,   outTypes);
    RegisterType(ImguiBuilder,       outTypes);
    
    //** Register Enums **//
    {
        static EnumEntry s_TextureType[] = {
            { "k1d",          (int)render::TextureType::k1d, "One dimensional texture. Width & depth defaults to 1." },
            { "k2d",          (int)render::TextureType::k2d, "Two dimensional texture. Depth defaults to 1." },
            { "k2dArray",     (int)render::TextureType::k2dArray, "Two dimensional array texture." },
            { "k3d",          (int)render::TextureType::k3d, "Volume texture type." },
            { "CubeMap",      (int)render::TextureType::CubeMap, "Cube map texture." },
            { "CubeMapArray", (int)render::TextureType::CubeMapArray, "Array of cube maps. Use the Depth as the number of cube maps supported in an array." },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumTextureType", "TextureType", s_TextureType, "Type of texture")); 
    }

    {
        static EnumEntry s_BufferType[] = {
            { "Standard",   (int)render::BufferType::Standard, "Standard buffer. Obeys the format specified when sampling / writting into it." },
            { "Structured", (int)render::BufferType::Structured, "Structured buffer, requires stride parameter." },
            { "Raw",        (int)render::BufferType::Raw, "Raw byte buffer." },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumBufferType", "BufferType", s_BufferType, "Type of buffer")); 
    }

    {
        static EnumEntry s_MemFlags[] = {
            { "GpuRead",  render::MemFlag_GpuRead, "Specify flag for resource read access from InResourceTable / SRVs" },
            { "GpuWrite", render::MemFlag_GpuWrite, "Specify flag for resource write access from an OutResourceTable / UAV" },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumMemFlags", "MemFlags", s_MemFlags, "Memory access enumerations.")); 
    }

    {
        static EnumEntry s_FilterTypes[] = {
            { "Point",        (int)render::FilterType::Point, "Point (nearest neighbor) filter type." },
            { "Linear",       (int)render::FilterType::Linear, "Bilinear/trilinear filter type." },
            { "Min",          (int)render::FilterType::Min, "Max value of neighborhood filter type." },
            { "Max",          (int)render::FilterType::Max, "Min value of neighborhood filter type." },
            { "Anisotropic",  (int)render::FilterType::Anisotropic, "High quality anisotropic filter type." },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.EnumFilterType", "FilterType", s_FilterTypes, "Filter types for samplers enumeration.")); 
    }

    {
        static EnumEntry s_TextureAddressingModes[] = {
            { "Wrap",   (int)render::TextureAddressMode::Wrap, "Repeats samples when UV coordinates exceed range." },
            { "Mirror", (int)render::TextureAddressMode::Mirror, "Applies UV mirroring when UV coordinates go into the next edge. " },
            { "Clamp",  (int)render::TextureAddressMode::Clamp, "Clamps the UV coordinates at the edges. " },
            { "Border", (int)render::TextureAddressMode::Border, "Samples a border when UVs are in the edge. Set the border color in the sampler object." },
            { nullptr, 0, nullptr }
        };
        outTypes.push_back(RenderEnum::constructEnumType("gpu.TextureAddressMode", "TextureAddressMode",  s_TextureAddressingModes, "Address behaviour of texture coordinates enumeration.")); 
    }

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
