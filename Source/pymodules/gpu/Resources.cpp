#include "Resources.h"
#include "ModuleState.h"
#include "CoalpyTypeObject.h"
#include <coalpy.render/IDevice.h>

namespace coalpy
{
namespace gpu
{

bool validateEnum(ModuleState& state, int value, int count, const char* name, const char* typeName)
{
    if (value >= count || value < 0)
    {
        PyErr_Format(state.exObj(),
            "Invalid enum value encountered %d in parameter %s. Range of this enum is [0,%d]. For valid values, see coalpy.gpu.%s",
            value, name, (count -1), typeName);
        return false;
    }

    return true;
}

void Buffer::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Buffer";
    t.tp_basicsize = sizeof(Buffer);
    t.tp_doc   = "";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Buffer::init;
    t.tp_dealloc = Buffer::destroy;
}

int Buffer::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    static char* arguments[] = { "name", "mem_flags", "type", "format", "element_count", "stride", nullptr };
    const char* name = "<unknown>";
    render::BufferDesc buffDesc;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|siiiii", arguments, &name, &buffDesc.memFlags, &buffDesc.type, &buffDesc.format, &buffDesc.elementCount, &buffDesc.stride))
        return -1;

    buffDesc.name = name;

    //validate
    if (!validateEnum(moduleState, (int)buffDesc.type, (int)render::BufferType::Count, "type", "BufferType"))
        return -1;
    if (!validateEnum(moduleState, (int)buffDesc.format, (int)Format::MAX_COUNT, "format", "Format"))
        return -1;

    auto* buffer = (Buffer*)self;
    buffer->buffer = moduleState.device().createBuffer(buffDesc);

    return 0;
}

void Buffer::destroy(PyObject* self)
{
    auto* buffer = (Buffer*)self;
    if (!buffer->buffer.valid())
        return;

    ModuleState& moduleState = parentModule(self);

    moduleState.device().release(buffer->buffer);
}

void Texture::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Texture";
    t.tp_basicsize = sizeof(Texture);
    t.tp_doc   = "";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Texture::init;
    t.tp_dealloc = Texture::destroy;
}

int Texture::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    auto* texture = (Texture*)self;
    render::TextureDesc desc;
    texture->texture = moduleState.device().createTexture(desc);
    return 0;
}

void Texture::destroy(PyObject* self)
{
    auto* texture = (Texture*)self;
    if (!texture->texture.valid())
        return;

    ModuleState& moduleState = parentModule(self);
    moduleState.device().release(texture->texture);
}


}
}
