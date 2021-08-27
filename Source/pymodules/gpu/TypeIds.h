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
    CommandList,
    Sampler,
    Buffer,
    InResourceTable,
    OutResourceTable,
    ImguiBuilder,
    Counts,

    //special type, reserved for enums
    GenericEnum
};


}
}
