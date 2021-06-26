#include "Resources.h"
#include "ModuleState.h"
#include "CoalpyTypeObject.h"
#include <coalpy.render/IDevice.h>

namespace coalpy
{
namespace gpu
{

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

    auto* buffer = (Buffer*)self;
    render::BufferDesc desc;
    buffer->buffer = moduleState.device().createBuffer(desc);

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
