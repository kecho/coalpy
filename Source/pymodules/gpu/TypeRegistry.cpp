#include "TypeRegistry.h"
#include "HelperMacros.h"
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "Resources.h"
#include "RenderEnum.h"
#include "CoalpyTypeObject.h"
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

void constructTypes(TypeList& outTypes)
{
    RegisterType(Window,     outTypes);
    RegisterType(Shader,     outTypes);
    RegisterType(RenderArgs, outTypes);
    RegisterType(Buffer,     outTypes);
    RegisterType(Texture,    outTypes);
    
    //register enums
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
