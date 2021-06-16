#include "TypeRegistry.h"
#include "HelperMacros.h"
#include "Window.h"
#include "Shader.h"
#include "RenderArgs.h"
#include "CoalpyTypeObject.h"

namespace coalpy
{
namespace gpu
{

void constructTypes(TypeList& outTypes)
{
    RegisterType(Window,     outTypes);
    RegisterType(Shader,     outTypes);
    RegisterType(RenderArgs, outTypes);
}

}
}
