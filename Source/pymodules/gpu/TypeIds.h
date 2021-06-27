#pragma once

namespace coalpy
{
namespace gpu
{

enum class TypeId : int
{
    Window,
    Shader,
    RenderArgs,
    Texture,
    Buffer,
    InResourceTable,
    OutResourceTable,
    Counts,

    //special type, reserved for enums
    GenericEnum
};


}
}
