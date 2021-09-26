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
    SamplerTable,
    ResourceDownloadRequest,
    ImguiBuilder,
    Counts,

    //special type, reserved for enums
    GenericEnum
};


}
}
