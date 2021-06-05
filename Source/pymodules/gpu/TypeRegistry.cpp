#include "TypeRegistry.h"
#include "HelperMacros.h"
#include "Window.h"

namespace coalpy
{
namespace gpu
{

void constructTypes(std::vector<PyTypeObject>& outTypes)
{
    RegisterType(Window, outTypes);
}

}
}
