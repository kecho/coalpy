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
    Counts,

    //special type, reserved for enums
    GenericEnum
};


}
}
