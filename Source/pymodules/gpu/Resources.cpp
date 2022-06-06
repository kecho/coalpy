#include "Resources.h"
#include "ModuleState.h"
#include "CoalpyTypeObject.h"
#include "PyUtils.h"
#include <coalpy.render/IDevice.h>
#include <coalpy.texture/ITextureLoader.h>

namespace coalpy
{
namespace gpu
{

bool validateEnum(ModuleState& state, int value, int count, const char* name, const char* typeName)
{
    if (value > count || value < 0)
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
    t.tp_doc   = R"(
    Class that represents a Buffer GPU resource.

    Constructor:
        name (str): string to identify the resource with.
        mem_flags (int): See coalpy.gpu.MemFlags. Default is Gpu_Read and Gpu_Write.
        type (int): See coalpy.gpu.BufferType. Default is Standard.
        format (int): Format of buffer. See coalpy.gpu.Format for available formats. This argument is ignored if buffer is Structured or Raw. Default format is RGBA_32_SINT
        element_count (int): number of elements this buffer will have
        stride (int): stride count in case of Structured type
        is_constant_buffer (Bool) : True if this can be used as a constant buffer. False otherwise.
        is_append_consume (Bool) : True if this can be used as an append consume buffer in the GPU. False otherwise. This wont work for raw buffers.
    )";

    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Buffer::init;
    t.tp_dealloc = Buffer::destroy;
}

int Buffer::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* buffer = (Buffer*)self;
    new (buffer) Buffer;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    static char* arguments[] = { "name", "mem_flags", "type", "format", "element_count", "stride", "is_constant_buffer", "is_append_consume", nullptr };
    const char* name = "<unknown>";
    render::BufferDesc buffDesc;
    int isConstantBuffer = 0;
    int isAppendConsume = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|siiiiipp", arguments,
            &name,
            &buffDesc.memFlags,
            &buffDesc.type,
            &buffDesc.format,
            &buffDesc.elementCount,
            &buffDesc.stride,
            &isConstantBuffer,
            &isAppendConsume))
        return -1;

    buffDesc.name = name;
    buffDesc.isConstantBuffer = isConstantBuffer;
    buffDesc.isAppendConsume  = isAppendConsume;
    buffer->isAppendConsume = isAppendConsume;

    //validate
    if (!validateEnum(moduleState, (int)buffDesc.type, (int)render::BufferType::Count, "type", "BufferType"))
        return -1;
    if (!validateEnum(moduleState, (int)buffDesc.format, (int)Format::MAX_COUNT, "format", "Format"))
        return -1;

    render::BufferResult buffResult = moduleState.device().createBuffer(buffDesc);
    if (!buffResult.success())
    {
        PyErr_Format(moduleState.exObj(), "Count not instantiate buffer object: %s", buffResult.message.c_str());
        return -1;
    }
    buffer->buffer = buffResult.object;
    return 0;
}

void Buffer::destroy(PyObject* self)
{
    auto* buffer = (Buffer*)self;
    if (!buffer->buffer.valid())
        return;

    ModuleState& moduleState = parentModule(self);

    moduleState.device().release(buffer->buffer);
    buffer->~Buffer();
    Py_TYPE(self)->tp_free(self);
}

void Texture::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Texture";
    t.tp_basicsize = sizeof(Texture);
    t.tp_doc   = R"(
    Class that represents a Texture GPU resource.

    Constructor:
        name (str): string to identify the resource with.
        mem_flags (int): see coalpy.gpu.MemFlags. Default is Gpu_Read and Gpu_Write
        type (int): dimensionality of texture, see coalpy.gpu.TextureType. Default is k2d
        format (int): Format of texture. See coalpy.gpu.Format for available formats.
        width (int): the width of the texture in texels. Default is 1
        height (int): the height of the texture in texels. Default is 1
        depth (int): the depth of the texture if k2dArray or k3d. Default is 1.
        mip_levels (int): number of mips supported on this texture.
        file (str): Load a texture file name (jpeg or png). All other parameters will be ignored when the file name set.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Texture::init;
    t.tp_dealloc = Texture::destroy;
}

int Texture::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* texture = (Texture*)self;
    new (texture) Texture;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    static char* arguments[] = { "name", "mem_flags", "type", "format", "width", "height", "depth", "mip_levels", "file", nullptr };
    const char* name = "<unknown>";
    const char* filename = nullptr;
    render::TextureDesc texDesc;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|siiiiiiis", arguments, &name, &texDesc.memFlags, &texDesc.type, &texDesc.format, &texDesc.width, &texDesc.height, &texDesc.depth, &texDesc.mipLevels, &filename))
        return -1;

    if (filename != nullptr)
    {
        ITextureLoader& tl = moduleState.tl();
        TextureLoadResult result = tl.loadTexture(filename);
        if (!result.success())
        {
            PyErr_Format(moduleState.exObj(), "Failed loading texture %s, reason: %s", filename, result.message.c_str());
            return -1;
        }

        texture->texture = result.texture;
        texture->isFile = true;
        return 0;
    }

    //validate
    if (!validateEnum(moduleState, (int)texDesc.type, (int)render::TextureType::Count, "type", "TextureType"))
        return -1;
    if (!validateEnum(moduleState, (int)texDesc.format, (int)Format::MAX_COUNT, "format", "Format"))
        return -1;

    render::TextureResult texResult = moduleState.device().createTexture(texDesc);
    if (!texResult.success())
    {
        PyErr_Format(moduleState.exObj(), "Count not instantiate texture object: %s", texResult.message.c_str());
        return -1;
    }
    texture->texture = texResult.object;
    return 0;
}

void Texture::destroy(PyObject* self)
{
    auto* texture = (Texture*)self;
    if (!texture->texture.valid() || !texture->owned)
        return;

    ModuleState& moduleState = parentModule(self);
    moduleState.onDestroyTexture(*texture);

    if (texture->isFile)
        moduleState.tl().unloadTexture(texture->texture);
    else
        moduleState.device().release(texture->texture);
    texture->~Texture();
    Py_TYPE(self)->tp_free(self);
}

void Sampler::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Sampler";
    t.tp_basicsize = sizeof(Sampler);
    t.tp_doc   = R"(
    Class that represents a Sampler GPU resource.

    Constructor:
        filter_type (int): see coalpy.gpu.FilterType. Sets the type of filter to use.
        address_u (int): see coalpy.gpu.TextureAddressMode. Sets behaviour on texture sampling when out of bounds for x axis.
        address_v (int): see coalpy.gpu.TextureAddressMode. Sets behaviour on texture sampling when out of bounds for y axis.
        address_w (int): see coalpy.gpu.TextureAddressMode. Sets behaviour on texture sampling when out of bounds for z axis.
        border_color (array): Must be an array of size 4 floats. Specifies the color in rgba format on border, when address mode is border. Ranges from 0.0 to 1.0 per channel.
        min_lod (float): Minimum texture mip.
        max_lod (float): Maximum texture mip.
        max_aniso (int): maximum quality for anisotropic filtering.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Sampler::init;
    t.tp_dealloc = Sampler::destroy;
}

int Sampler::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* sampler = (Sampler*)self;
    new (sampler) Sampler;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    static char* arguments[] = { "filter_type", "address_u", "address_v", "address_w", "border_color", "min_lod", "max_lod", "max_aniso", nullptr };
    const char* name = "<unknown>";
    PyObject* borderColorObj = nullptr;
    render::SamplerDesc samplerDesc;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|iiiiOiii", arguments, &samplerDesc.type, &samplerDesc.addressU, &samplerDesc.addressV, &samplerDesc.addressW, &borderColorObj, &samplerDesc.minLod, &samplerDesc.maxLod, &samplerDesc.maxAnisoQuality))
        return -1;
    if (!validateEnum(moduleState, (int)samplerDesc.type, (int)render::FilterType::Count, "filter_type", "FilterType"))
        return -1;
    if (!validateEnum(moduleState, (int)samplerDesc.addressU, (int)render::TextureAddressMode::Count, "address_u", "TextureAddressMode"))
        return -1;
    if (!validateEnum(moduleState, (int)samplerDesc.addressV, (int)render::TextureAddressMode::Count, "address_v", "TextureAddressMode"))
        return -1;
    if (!validateEnum(moduleState, (int)samplerDesc.addressW, (int)render::TextureAddressMode::Count, "address_w", "TextureAddressMode"))
        return -1;
    if (!validateEnum(moduleState, (int)samplerDesc.minLod, 8, "min_lod", ""))
        return -1;
    if (!validateEnum(moduleState, (int)samplerDesc.maxLod, 8, "max_lod", ""))
        return -1;

    std::vector<int> nums;
    if (borderColorObj)
    {
        if (!getArrayOfNums(moduleState, borderColorObj, nums, true, false) || nums.size() != 4)
        {
            PyErr_SetString(moduleState.exObj(), "border_color must be an array of exactly 4 float values, from 0 to 1 representing an RGBA value");
            return -1;
        }

        const float* bordCol = reinterpret_cast<float*>(nums.data());
        for (int i = 0; i < 4; ++i)
            samplerDesc.borderColor[i] = bordCol[i];
    }
    else
        for (int i = 0; i < 4; ++i)
            samplerDesc.borderColor[i] = 0.0f;

    render::SamplerResult samplerResult = moduleState.device().createSampler(samplerDesc);
    if (!samplerResult.success())
    {
        PyErr_Format(moduleState.exObj(), "Could not create sampler. Issue: %s", samplerResult.message.c_str());
        return -1;
    }

    sampler->sampler = samplerResult;
    return 0;
}

void Sampler::destroy(PyObject* self)
{
    auto* sampler = (Sampler*)self;
    if (!sampler->sampler.valid())
        return;

    ModuleState& moduleState = parentModule(self);
    moduleState.device().release(sampler->sampler);
    sampler->~Sampler();
    Py_TYPE(self)->tp_free(self);
}

void InResourceTable::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.InResourceTable";
    t.tp_basicsize = sizeof(InResourceTable);
    t.tp_doc   = R"(
    Input resource table. Use this to specify the inputs of a dispatch call in CommandList.

    Constructor:
        name (str): name / identifier of this resource table.
        resource_list: array of Texture or Buffer to be specified. Each position in the table correspods to a t# register in hlsl.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = InResourceTable::init;
    t.tp_dealloc = InResourceTable::destroy;
}

static bool convertToResourceHandleList(ModuleState& moduleState, PyObject* resourceList, std::vector<render::ResourceHandle>& output)
{
    if (resourceList == nullptr || !PyList_Check(resourceList))
    {
        PyErr_SetString(moduleState.exObj(), "Expected a list object.");
        return false;
    }
    Py_ssize_t listSize = (int)PyList_Size(resourceList);
    auto& listObject = *((PyListObject*)resourceList);
    if (listSize == 0)
    {
        PyErr_SetString(moduleState.exObj(), "Expected a non empty list object.");
        return false;
    }

    CoalpyTypeObject* textureType = moduleState.getCoalpyType(TypeId::Texture);
    CoalpyTypeObject* bufferType  = moduleState.getCoalpyType(TypeId::Buffer);

    for (int i = 0; i < (int)listSize; ++i)
    {
        PyObject* element = listObject.ob_item[i];
        if (element == nullptr)
        {
            PyErr_Format(moduleState.exObj(), "Null object found in list at index %d.", i);
            return false;
        }

        render::ResourceHandle handle;
        if (element->ob_type == (PyTypeObject*)bufferType)
        {
            auto* buffer = (Buffer*)element;
            handle = buffer->buffer;
        }
        else if (element->ob_type == (PyTypeObject*)textureType)
        {
            auto* texture = (Texture*)element;
            handle = texture->texture;
        }
        else
        {
            PyErr_Format(moduleState.exObj(), "List only allows types coalpy.gpu.Texture and coalpy.gpu.Buffer. Invalid Object found at index %d.", i);
            return false;
        }

        if (!handle.valid())
        {
            PyErr_Format(moduleState.exObj(), "Resource found at index %d is invalid.", i);
            return false;
        }

        output.push_back(handle);
    }

    return true;
}

int InResourceTable::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* newTable = (InResourceTable*)self;
    new (newTable) InResourceTable;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    PyObject* resourceList = nullptr;
    const char* name = "<unknown>";
    static char* arguments[] = { "name", "resource_list", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO", arguments, &name, &resourceList))
        return -1;

    std::vector<render::ResourceHandle> resources;
    if (!convertToResourceHandleList(moduleState, resourceList, resources))
        return -1;

    render::ResourceTableDesc tableDesc;
    tableDesc.name = name;
    tableDesc.resources = resources.data();
    tableDesc.resourcesCount = (int)resources.size();
    render::InResourceTableResult tableResult = moduleState.device().createInResourceTable(tableDesc);
    if (!tableResult.success())
    {
        PyErr_Format(moduleState.exObj(), "Error creating resource table %s with error: %s", name, tableResult.message.c_str());
        return -1;
    }

    newTable->table = tableResult.object;

    if (!newTable->table.valid())
    {
        PyErr_SetString(moduleState.exObj(), "Internal error while creating table. Check error stream for rendering errors.");
        return -1;
    }

    return 0;
}

void InResourceTable::destroy(PyObject* self)
{
    auto* table = (InResourceTable*)self;
    if (!table->table.valid())
        return;

    ModuleState& moduleState = parentModule(self);
    moduleState.device().release(table->table);
    table->~InResourceTable();
    Py_TYPE(self)->tp_free(self);
}

void OutResourceTable::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.OutResourceTable";
    t.tp_basicsize = sizeof(OutResourceTable);
    t.tp_doc   = R"(
    Output resource table. Use this to specify a table of unordered access views (outputs) of a dispatch call in CommandList.

    Constructor:
        name (str): name / identifier of this resource table.
        resource_list: array of Texture or Buffer to be specified. Each position in the table correspods to a u# register in hlsl.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = OutResourceTable::init;
    t.tp_dealloc = OutResourceTable::destroy;
}

int OutResourceTable::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* newTable = (OutResourceTable*)self;
    new (newTable) OutResourceTable;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    PyObject* resourceList = nullptr;
    const char* name = "<unknown>";
    static char* arguments[] = { "name", "resource_list", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO", arguments, &name, &resourceList))
        return -1;

    std::vector<render::ResourceHandle> resources;
    if (!convertToResourceHandleList(moduleState, resourceList, resources))
        return -1;

    render::ResourceTableDesc tableDesc;
    tableDesc.name = name;
    tableDesc.resources = resources.data();
    tableDesc.resourcesCount = (int)resources.size();
    render::OutResourceTableResult tableResult = moduleState.device().createOutResourceTable(tableDesc);
    if (!tableResult.success())
    {
        PyErr_Format(moduleState.exObj(), "Error creating resource table %s with error: %s", name, tableResult.message.c_str());
        return -1;
    }

    newTable->table = tableResult.object;

    if (!newTable->table.valid())
    {
        PyErr_SetString(moduleState.exObj(), "Internal error while creating table. Check error stream for rendering errors.");
        return -1;
    }

    return 0;
}

void OutResourceTable::destroy(PyObject* self)
{
    auto* table = (OutResourceTable*)self;
    if (!table->table.valid())
        return;

    ModuleState& moduleState = parentModule(self);
    moduleState.device().release(table->table);
    table->~OutResourceTable();
    Py_TYPE(self)->tp_free(self);
}

void SamplerTable::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.SamplerTable";
    t.tp_basicsize = sizeof(SamplerTable);
    t.tp_doc   = R"(
    Sampler table. Use this to specify a table of Samplers to be used in a CommandList and shader.

    Constructor:
        name (str): name / identifier of this sampler table.
        samplers: array of Samplers to be specified.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = SamplerTable::init;
    t.tp_dealloc = SamplerTable::destroy;
}

int SamplerTable::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* newTable = (SamplerTable*)self;
    new (newTable) SamplerTable;

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    PyObject* resourceList = nullptr;
    const char* name = "<unknown>";
    static char* arguments[] = { "name", "samplers", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO", arguments, &name, &resourceList))
        return -1;

    std::vector<render::ResourceHandle> resources;
    if (!convertToResourceHandleList(moduleState, resourceList, resources))
        return -1;

    render::ResourceTableDesc tableDesc;
    tableDesc.name = name;
    tableDesc.resources = resources.data();
    tableDesc.resourcesCount = (int)resources.size();
    render::SamplerTableResult tableResult = moduleState.device().createSamplerTable(tableDesc);
    if (!tableResult.success())
    {
        PyErr_Format(moduleState.exObj(), "Error creating sampler table %s with error: %s", name, tableResult.message.c_str());
        return -1;
    }

    newTable->table = tableResult.object;

    if (!newTable->table.valid())
    {
        PyErr_SetString(moduleState.exObj(), "Internal error while creating table. Check error stream for rendering errors.");
        return -1;
    }

    return 0;
}

void SamplerTable::destroy(PyObject* self)
{
    auto* table = (SamplerTable*)self;
    if (!table->table.valid())
        return;

    ModuleState& moduleState = parentModule(self);
    moduleState.device().release(table->table);
    table->~SamplerTable();
    Py_TYPE(self)->tp_free(self);
}


}
}
