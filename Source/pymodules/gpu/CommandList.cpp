#include "CommandList.h"
#include "ModuleState.h"
#include "Shader.h"
#include "CoalpyTypeObject.h"
#include "HelperMacros.h"
#include "Resources.h"
#include "TypeIds.h"
#include "PyUtils.h"
#include <longobject.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/Resources.h>
#include <coalpy.render/CommandList.h>

namespace coalpy
{
namespace gpu
{

namespace methods
{
    static PyObject* cmdDispatch(PyObject* self, PyObject* vargs, PyObject* kwds);
    static PyObject* cmdCopyResource(PyObject* self, PyObject* vargs, PyObject* kwds);
    static PyObject* cmdUploadResource(PyObject* self, PyObject* vargs, PyObject* kwds);
    static PyObject* cmdCopyAppendConsumeCounter(PyObject* self, PyObject* vargs, PyObject* kwds);
    static PyObject* cmdClearAppendConsume(PyObject* self, PyObject* vargs, PyObject* kwds);
    static PyObject* cmdBeginMarker(PyObject* self, PyObject* vargs, PyObject* kwds);
    static PyObject* cmdEndMarker(PyObject* self, PyObject* vargs, PyObject* kwds);
}

static PyMethodDef g_cmdListMethods[] = {
    KW_FN(dispatch, cmdDispatch,R"(
        Dispatch method, which executes a compute shader with resources bound.
        
        Parameters:
            x (int)(optional): the number of groups on the x axis. By default is 1.
            y (int)(optional): the number of groups on the y axis. By default is 1.
            z (int)(optional):  the number of groups on the z axis. By default
            shader (Shader): object of type Shader. This will be the compute shader launched on the GPU.
            name (str)(optional): Debug name of this dispatch to see in render doc / profiling captures.
            constants (optional): Constant can be, an array of ints and floats, an array.array
                                  or any object compatible with the object protocol, or a list of Buffer objects.
                                  If a list of Buffer objects, these can be indexed via register(b#) in the shader used,
                                  Every other type will always be bound to a constant Buffer on register(b0).
            samplers (optional): a single Sampler object, or an array of Sampler objects, or a single SamplerTable object, or an array of SamplerTable objects. If a single
                                   SamplerTable object is used, the table will be automatically bound to register space 0, and each resource accessed either
                                   by bindless dynamic indexing, or a hard register(s#). If an array of SamplerTable is passed, each resource
                                   table will be bound using a register space index corresponding to the table index in the array, and the rules
                                   to reference each sampler within the table stay the same.
                                   If and array of Sampler objects are passed (or a single Sampler object), each sampler object is bound to a single register and always to the default space (0).
            inputs (optional): a single InResourceTable object, or an array of InResourceTable objects, or a single Texture/Buffer object, or an array of Texture/Buffer objects. If a single
                                   object is used, the table will be automatically bound to register space 0, and each resource accessed either
                                   by bindless dynamic indexing, or a hard register(t#). If an array of InResourceTable is passed, each resource
                                   table will be bound using a register space index corresponding to the table index in the array, and the rules
                                   to reference each resource within the table stay the same.
                                   If and array of Texture/Buffer objects are passed (or a single Texture/Buffer object), each object is bound to a single register and always to the default space (0).
            outputs (optional): a single OutResourceTable object, or an array of OutResourceTable objects. Same rules as inputs apply,
                                     but we use registers u# to address the UAVs.

            indirect_args (Buffer)(optional): a single object of type Buffer, which contains the x, y and z groups packed tightly as 3 ints. 
                                      If this buffer is provided, the x, y and z arguments are ignored.
    )"),
    KW_FN(copy_resource, cmdCopyResource, R"(
        copy_resource method, copies one resource to another.
        Both source and destination must be the same type (either Buffer or textures).
        Parameters:
            source (Texture or Buffer): an object of type Texture or Buffer. This will be the source resource to copy from.
            destination (Texture or Buffer): an object of type Texture or Buffer. This will be the destination resource to copy to.
            source_offset (tuple or int): if texture copy, a tuple with x, y, z offsets and mipLevel must be specified. If Buffer copy, it must be a single integer with the byte offset for the source Buffer.
            destination_offset (tuple or int): if texture copy, a tuple with x, y, z offsets and mipLevel must be specified. If Buffer copy, it must be a single integer with the byte offset for the destianation Buffer.
            size (tuple or int): if texture a tuple with the x, y and z size. If Buffer just an int with the proper byte size. Default covers the entire size.
    )"),
    KW_FN(upload_resource, cmdUploadResource, R"(
        upload_resource method. Uploads an python array [], an array.array or any buffer protocol compatible object to a gpu resource.
        
        Parameters:
            source: an array of ints and floats, or an array.array object or any object compatible with the buffer protocol (for example a bytearray).
            destination (Texture or Buffer): a destination object of type Texture or Buffer.
            size (tuple): if texture upload, a tuple with the x, y and z size of the box to copy of the source buffer. If a Buffer upload, then this parameter gets ignored.
            destination_offset (tuple or int): if texture copy, a tuple with x, y, z offsets and mipLevel must be specified. If Buffer copy, it must be a single integer with the byte offset for the destianation Buffer.
    )"),
    KW_FN(clear_append_consume_counter, cmdClearAppendConsume, R"(
        Clears the append consume counter of a Buffer that was created with the is_append_consume flag set to true.

        Parameters:
            source (Buffer): source Buffer object
            clear_value (int)(optional): integer value with the clear count. By default is 0.
    )"),
    KW_FN(copy_append_consume_counter, cmdCopyAppendConsumeCounter, R"(
        Copies the counter of an append consume buffer to a destination resource.
        The destination must be a buffer that holds at least 4 bytes (considering the offset).

        Parameters:
            source (Buffer): source Buffer object, must have is_append_consume flag to True.
            destination (Buffer): Destination buffer to hold value.
            destination_offset (int)(optional): integer value with the destination's buffer offset, in bytes.
    )"),
    KW_FN(begin_marker, cmdBeginMarker, R"(
        Sets a string marker. Must be paired with a call to end_marker. Markers can also be nested.
        You can use renderdoc / pix to explore the resulting frame's markers.
        
        Parameters:
            name (str): string text of the marker to be used.
    )"),
    KW_FN(end_marker, cmdEndMarker, R"(
        Finishes the scope of a marker. Must have a corresponding begin_marker.
    )"),
    FN_END
};

void CommandList::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.CommandList";
    t.tp_basicsize = sizeof(CommandList);
    t.tp_doc   = "Class that represents a stream of commands. "
                 "Takes no arguments for constructor. CommandList objects can be uploaded\n"
                 "via coalpy.gpu.schedule. CommandList objects can be resubmitted as many times\n"
                 "and can be reused to avoid cost.";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = CommandList::init;
    t.tp_dealloc = CommandList::destroy;
    t.tp_methods = g_cmdListMethods;
}

int CommandList::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    auto& pycmdList = *((CommandList*)self);
    new (&pycmdList) CommandList;
    pycmdList.cmdList = moduleState.newCommandList();

    return 0;
}

void CommandList::destroy(PyObject* self)
{
    ModuleState& moduleState = parentModule(self);
    auto& pycmdList = *((CommandList*)self);
    moduleState.deleteCommandList(pycmdList.cmdList);

    for (auto* r : pycmdList.references.objects)
        Py_DECREF(r);

    for (render::ResourceTable tmpTable : pycmdList.references.tmpTables)
        moduleState.device().release(tmpTable);

    pycmdList.~CommandList();
    Py_TYPE(self)->tp_free(self);
}

static bool getListOfBuffers(
    ModuleState& moduleState,
    PyObject* opaqueList,
    std::vector<render::Buffer>& bufferList,
    CommandListReferences& references)
{
    PyTypeObject* bufferType = moduleState.getType(TypeId::Buffer);
    if (!PyList_Check(opaqueList))
    {
        if (opaqueList->ob_type == bufferType)
        {
            Buffer& buff = *((Buffer*)opaqueList);
            bufferList.push_back(buff.buffer);
            references.objects.push_back(opaqueList);
            return true;
        }
        return false;
    }

    auto& listObj = *((PyListObject*)opaqueList);
    int listSize = Py_SIZE(opaqueList);
    for (int i = 0; i < listSize; ++i)
    {
        PyObject* obj = listObj.ob_item[i];
        if (obj->ob_type != bufferType)
            return false;

        Buffer& buff = *((Buffer*)obj);
        bufferList.push_back(buff.buffer);
        references.objects.push_back(obj);
    }

    return true;
}

enum class TmpTableFunction
{
    Sampler, Input, Output
};

template<typename PyTableType, typename TableType, TmpTableFunction tmpTableFn>
static bool getListOfTables(
    ModuleState& moduleState,
    PyObject* opaqueList,
    std::vector<TableType>& bufferList,
    CommandListReferences& references)
{
    PyTypeObject* pyTableType = moduleState.getType(PyTableType::s_typeId);
    if (opaqueList == Py_None)
        return true;

    if (!PyList_Check(opaqueList))
    {
        if (opaqueList->ob_type == pyTableType)
        {
            PyTableType& tobj = *((PyTableType*)opaqueList);
            bufferList.push_back(tobj.table);
            references.objects.push_back(opaqueList);
            return true;
        }
        else if (tmpTableFn == TmpTableFunction::Input || tmpTableFn == TmpTableFunction::Output)
        {
            PyTypeObject* bufferType = moduleState.getType(Buffer::s_typeId);
            PyTypeObject* textureType = moduleState.getType(Texture::s_typeId);

            render::ResourceHandle resource;
            if (opaqueList->ob_type == bufferType)
            {
                resource = ((Buffer*)(opaqueList))->buffer;
            }
            else if (opaqueList->ob_type == textureType)
            {
                resource = ((Texture*)(opaqueList))->texture;
            }
            else return false;

            render::ResourceTableDesc tableDesc;
            tableDesc.name = "tmpTable";
            render::ResourceTable tmpResourceTable;
            tableDesc.resources = &resource;
            tableDesc.resourcesCount = 1;
            if (tmpTableFn == TmpTableFunction::Input)
            {
                auto result = moduleState.device().createInResourceTable(tableDesc);
                if (!result.success())
                {
                    PyErr_SetString(moduleState.exObj(), result.message.c_str());
                    return false;
                }
                tmpResourceTable = result.object;
            }
            else 
            {
                auto result = moduleState.device().createOutResourceTable(tableDesc);
                if (!result.success())
                {
                    PyErr_SetString(moduleState.exObj(), result.message.c_str());
                    return false;
                }
                tmpResourceTable = result.object;
            }

            bufferList.push_back(TableType { tmpResourceTable.handleId });
            references.tmpTables.push_back(tmpResourceTable);
            return true;
        }
        else if (tmpTableFn == TmpTableFunction::Sampler)
        {
            PyTypeObject* samplerType = moduleState.getType(Sampler::s_typeId);
            render::ResourceHandle resource;
            if (opaqueList->ob_type == samplerType)
            {
                resource = ((Sampler*)opaqueList)->sampler;
            }
            else return false;

            render::ResourceTableDesc tableDesc;
            tableDesc.name = "tmpTable";
            render::ResourceTable tmpResourceTable;
            tableDesc.resources = &resource;
            tableDesc.resourcesCount = 1;
            auto result = moduleState.device().createSamplerTable(tableDesc);
            if (!result.success())
            {
                PyErr_SetString(moduleState.exObj(), result.message.c_str());
                return false;
            }

            tmpResourceTable = result.object;
            bufferList.push_back(TableType { tmpResourceTable.handleId });
            references.tmpTables.push_back(tmpResourceTable);
            return true;
        }
        return false;
    }

    auto& listObj = *((PyListObject*)opaqueList);
    int listSize = Py_SIZE(opaqueList);
    if (listSize <= 0)
        return true;

    //initial case, we have and expect a list of explicit tables
    if (listObj.ob_item[0]->ob_type == pyTableType)
    {
        for (int i = 0; i < listSize; ++i)
        {
            PyObject* obj = listObj.ob_item[i];
            if (obj->ob_type != pyTableType)
                return false;

            PyTableType& tobj = *((PyTableType*)obj);
            bufferList.push_back(tobj.table);
            references.objects.push_back(obj);
        }
    }
    else
    {
        //secondary case, we have a list of resources.
        std::vector<render::ResourceHandle> resources;
        render::ResourceTableDesc tableDesc;

        if (tmpTableFn == TmpTableFunction::Input || tmpTableFn == TmpTableFunction::Output)
        {
            PyTypeObject* bufferType = moduleState.getType(Buffer::s_typeId);
            PyTypeObject* textureType = moduleState.getType(Texture::s_typeId);

            for (int i = 0; i < listSize; ++i)
            {
                PyObject* obj = listObj.ob_item[i];
                if (obj->ob_type == bufferType)
                {
                    resources.push_back(((Buffer*)obj)->buffer);
                    references.objects.push_back(obj);
                }
                else if (obj->ob_type == textureType)
                {
                    resources.push_back(((Texture*)obj)->texture);
                    references.objects.push_back(obj);
                }
                else
                    return false;
            }
        }
        else
        {
            PyTypeObject* samplerType = moduleState.getType(Sampler::s_typeId);
            for (int i = 0; i < listSize; ++i)
            {
                PyObject* obj = listObj.ob_item[i];
                if (obj->ob_type == samplerType)
                {
                    resources.push_back(((Sampler*)obj)->sampler);
                    references.objects.push_back(obj);
                }
                else
                    return false;
            }
        }

        tableDesc.name = "tmpTable";
        tableDesc.resources = resources.data();
        tableDesc.resourcesCount = resources.size();
        
        render::ResourceTable tmpResourceTable;
        if (tmpTableFn == TmpTableFunction::Input)
        {
            auto result = moduleState.device().createInResourceTable(tableDesc);
            if (!result.success())
            {
                PyErr_SetString(moduleState.exObj(), result.message.c_str());
                return false;
            }
            tmpResourceTable = result.object;
        }
        else if (tmpTableFn == TmpTableFunction::Output)
        {
            auto result = moduleState.device().createOutResourceTable(tableDesc);
            if (!result.success())
            {
                PyErr_SetString(moduleState.exObj(), result.message.c_str());
                return false;
            }
            tmpResourceTable = result.object;
        }
        else if (tmpTableFn == TmpTableFunction::Sampler)
        {
            auto result = moduleState.device().createSamplerTable(tableDesc);
            if (!result.success())
            {
                PyErr_SetString(moduleState.exObj(), result.message.c_str());
                return false;
            }
            tmpResourceTable = result.object;
        }

        bufferList.push_back(TableType { tmpResourceTable.handleId });
        references.tmpTables.push_back(tmpResourceTable);
    }

    return true;
}

static bool getBufferProtocolObject(
    ModuleState& moduleState,
    PyObject* constants, 
    char*& outBufferProtocolPtr, 
    int& outBufferProtocolSize, 
    std::vector<Py_buffer>& references)
{
    if (!PyObject_CheckBuffer(constants))
        return false;

    references.emplace_back();
    auto& view = references.back();
    if (PyObject_GetBuffer(constants, &view, 0) == -1)
    {
        references.pop_back();
        return false;
    }
    outBufferProtocolPtr = (char*)view.buf;
    outBufferProtocolSize = (int)view.len;
    return true;
}

static bool getResourceObject(ModuleState& moduleState, PyObject* obj, render::ResourceHandle& handle, bool& isBuffer)
{
    auto* texType = moduleState.getType(Texture::s_typeId);
    auto* buffType = moduleState.getType(Buffer::s_typeId);
    isBuffer = false;
    if (texType == obj->ob_type)
    {
        auto* texObj = (Texture*)obj;
        handle = texObj->texture;
        return true;
    }
    else if (buffType == obj->ob_type)
    {
        isBuffer = true;
        auto* buffObj = (Buffer*)obj;
        handle = buffObj->buffer;
        return true;
    }

    return false;
}

namespace methods
{
    PyObject* cmdDispatch(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        static char* arguments[] = { "shader", "x", "y", "z", "name", "constants", "samplers", "inputs", "outputs", "indirect_args", nullptr };
        int x = 1;
        int y = 1;
        int z = 1;
        const char* name = nullptr;
        PyObject* shader = nullptr;
        PyObject* constants = nullptr;
        PyObject* sampler_tables = nullptr;
        PyObject* input_tables = nullptr;
        PyObject* output_tables = nullptr;
        PyObject* indirect_args = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "O|iiisOOOOO", arguments, &shader, &x, &y, &z, &name, &constants, &sampler_tables, &input_tables, &output_tables, &indirect_args))
            return nullptr;
        
        if (x <= 0 || y <= 0 || z <= 0)
        {
            PyErr_SetString(moduleState.exObj(), "x, y and z arguments of dispatch must be greater or equal to 1");
            return nullptr;
        }

        CommandListReferences references;
        auto& cmdList = *(CommandList*)self;

        render::ComputeCommand cmd;

        if (indirect_args != nullptr)
        {
            bool srcIsBuffer = false;
            render::ResourceHandle indirectArgsHandle;
            if (!getResourceObject(moduleState, indirect_args, indirectArgsHandle, srcIsBuffer) || !indirectArgsHandle.valid())
            {
                PyErr_SetString(moduleState.exObj(), "indirect_args for dispatch command must be of type Buffer, and this buffer must be valid.");
                return nullptr;
            }

            cmd.setIndirectDispatch(name ? name : "", render::Buffer { indirectArgsHandle.handleId });
        }
        else
        {
            cmd.setDispatch(name ? name : "", x, y, z);
        }

        //for constants / inline constants. Must be in this scope, because these pointers are needed when writeCommand is called / flushed.
        std::vector<Py_buffer> bufferViews;
        std::vector<int> rawNums;
        std::vector<render::Buffer> bufferList;
        std::vector<render::InResourceTable> inTables;
        std::vector<render::OutResourceTable> outTables;
        std::vector<render::SamplerTable> samplerTables;

        PyTypeObject* shaderType = moduleState.getType(Shader::s_typeId);
        if (shader->ob_type != shaderType)
        {
            PyErr_SetString(moduleState.exObj(), "shader parameter must be of type coalpy.gpu.Shader");
            return nullptr;
        }

        Shader& shaderObj = *((Shader*)shader);
        references.objects.push_back(shader);
        cmd.setShader(shaderObj.handle);

        if (constants)
        {
            char* bufferProtocolPtr = nullptr;
            int bufferProtocolSize = 0;
            if (getListOfBuffers(moduleState, constants, bufferList, references))
            {
                if (!bufferList.empty())
                    cmd.setConstants(bufferList.data(), bufferList.size());
            }
            else if (getBufferProtocolObject(moduleState, constants, bufferProtocolPtr, bufferProtocolSize, bufferViews))
            {
                if (bufferProtocolPtr != nullptr)
                    cmd.setInlineConstant(bufferProtocolPtr, bufferProtocolSize);
            }
            else 
            {
                if (!getArrayOfNums(moduleState, constants, rawNums))
                {
                    PyErr_SetString(moduleState.exObj(), "Constant buffer must be: a list of Buffer objects, an array of [int|float], an array.array() or any object that follows the python Buffer protocol.");
                    return nullptr;
                }

                if (rawNums.empty())
                {
                    PyErr_SetString(moduleState.exObj(), "Constant buffer list cannot be empty");
                    return nullptr;
                }

                cmd.setInlineConstant((const char*)rawNums.data(), (int)rawNums.size() * (int)sizeof(int));
            }
        }

        if (sampler_tables)
        {
            if (!getListOfTables<SamplerTable, render::SamplerTable, TmpTableFunction::Sampler>(moduleState, sampler_tables, samplerTables, references))
            {
                PyErr_SetString(moduleState.exObj(), "samplers argument must be a list of SamplerTable, or a single SamplerTable, or a list of Samplers or a single Sampler");
                return nullptr;
            }

            if (!samplerTables.empty())
            {
                cmd.setSamplers(samplerTables.data(), (int)samplerTables.size());
            }
        }

        if (input_tables)
        {
            if (!getListOfTables<InResourceTable, render::InResourceTable, TmpTableFunction::Input>(moduleState, input_tables, inTables, references))
            {
                PyErr_SetString(moduleState.exObj(), "inputs argument must be a list of InResourceTable, or a single InResourceTable, or a list of resources, or a single resource");
                return nullptr;
            }

            if (!inTables.empty())
            {
                cmd.setInResources(inTables.data(), (int)inTables.size());
            }
        }

        if (output_tables)
        {
            if (!getListOfTables<OutResourceTable, render::OutResourceTable, TmpTableFunction::Output>(moduleState, output_tables, outTables, references))
            {
                PyErr_SetString(moduleState.exObj(), "outputs argument must be a list of OutResourceTable, or a single OutResourceTable, or a list of resources, or a single resource");
                return nullptr;
            }

            if (!outTables.empty())
            {
                cmd.setOutResources(outTables.data(), (int)outTables.size());
            }
        }

        cmdList.cmdList->writeCommand(cmd);
        for (auto* obj : references.objects)
            Py_INCREF(obj);

        cmdList.references.append(references);

        for (auto& v : bufferViews)
            PyBuffer_Release(&v);

        Py_RETURN_NONE;
    }

    PyObject* cmdCopyResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        auto& cmdList = *((CommandList*)self); 
        static char* arguments[] = { "source", "destination", "source_offset", "destination_offset", "size", nullptr };
        PyObject* source = nullptr;
        PyObject* destination = nullptr;
        PyObject* sourceOffsetObj = nullptr;
        PyObject* destinationOffsetObj = nullptr;
        PyObject* sizeObj = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "OO|OOO", arguments, &source, &destination, &sourceOffsetObj, &destinationOffsetObj, &sizeObj))
            return nullptr;

        render::ResourceHandle sourceHandle;
        render::ResourceHandle destHandle;
        bool srcIsBuffer = false;
        if (!getResourceObject(moduleState, source, sourceHandle, srcIsBuffer))
        {
            PyErr_SetString(moduleState.exObj(), "Source resource must be type texture or buffer");
            return nullptr;
        }

        bool destIsBuffer = false;
        if (!getResourceObject(moduleState, destination, destHandle, destIsBuffer))
        {
            PyErr_SetString(moduleState.exObj(), "Destination resource must be type texture or buffer");
            return nullptr;
        }

        if (destIsBuffer != srcIsBuffer)
        {
            PyErr_Format(moduleState.exObj(), "Destination and source must be both either a texture or a buffer. Source is a %s and destination a %s. ", srcIsBuffer ? "buffer" : "texture", destIsBuffer ? "buffer" : "texture");
            return nullptr;
        }

        if (destIsBuffer)
        {
            int byteSize = -1;  
            int sourceOffset = 0;  
            int destinationOffset = 0;  
            if (sourceOffsetObj != nullptr)
            {
                if (!PyLong_Check(sourceOffsetObj))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for source_offset must be an integer when performing buffer to buffer copies.");
                    return nullptr;
                }
        
                sourceOffset = (int)PyLong_AsLong(sourceOffsetObj);
            }

            if (destinationOffsetObj != nullptr)
            {
                if (!PyLong_Check(destinationOffsetObj))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for destination_offset must be an integer when performing buffer to buffer copies.");
                    return nullptr;
                }

                destinationOffset = (int)PyLong_AsLong(destinationOffsetObj);
            }

            if (sizeObj != nullptr)
            {
                if (!PyLong_Check(sizeObj))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for size must be an integer when performing buffer to buffer copies.");
                    return nullptr;
                }

                byteSize = (int)PyLong_AsLong(sizeObj);
            }

            render::CopyCommand cmd;
            cmd.setBuffers(
                render::Buffer { sourceHandle.handleId }, render::Buffer { destHandle.handleId},
                byteSize, sourceOffset, destinationOffset);
            cmdList.cmdList->writeCommand(cmd);
        }
        else
        {
            int sourceArgs[4] = { 0, 0, 0, 0 }; //sourceX, sourceY, sourceZ and srcMipLevel
            if (sourceOffsetObj != nullptr)
            {
                if (!getTupleValues(sourceOffsetObj, sourceArgs, 1, 4))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for source_offset must be a tuple or list of 1 to 4 ints [sourceX, sourceY, sourceZ, sourceMipLevel] when performing texture to texture copies.");
                    return nullptr;
                }
            }

            int destinationArgs[4] = { 0, 0, 0, 0 }; //destX, destY, destZ and destMipLevel
            if (destinationOffsetObj != nullptr)
            {
                if (!getTupleValues(destinationOffsetObj, destinationArgs, 1, 4))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for destination_offset must be a tuple or list of 1 to 4 ints [destX, destY, destZ, destMipLevel] when performing texture to texture copies.");
                    return nullptr;
                }
            }

            int sizesArgs[3] = { -1, -1, -1 }; //sizeX, sizeY, sizeZ
            if (sizeObj != nullptr)
            {
                if (!getTupleValues(sizeObj, sizesArgs, 1, 3))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for size must be a tuple or list of 1 to 3 ints [sizeX, sizeY, sizeZ] when performing texture to texture copies.");
                    return nullptr;
                }
            }

            render::CopyCommand cmd;
            cmd.setTextures( render::Texture { sourceHandle.handleId }, render::Texture { destHandle.handleId },
                sizesArgs[0], sizesArgs[1], sizesArgs[2],
                sourceArgs[0], sourceArgs[1], sourceArgs[2],
                destinationArgs[0], destinationArgs[1], destinationArgs[2],
                sourceArgs[3], destinationArgs[3]);
            cmdList.cmdList->writeCommand(cmd);
        }

        Py_INCREF(source);
        cmdList.references.objects.push_back(source);
        Py_INCREF(destination);
        cmdList.references.objects.push_back(destination);
        

        Py_RETURN_NONE;
    }

    PyObject* cmdUploadResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        auto& cmdList = *((CommandList*)self); 
        static char* arguments[] = { "source", "destination", "size", "destination_offset", nullptr };
        PyObject* source = nullptr;
        PyObject* destination = nullptr;
        PyObject* sizeObj = nullptr;
        PyObject* destinationOffsetObj = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "OO|OO", arguments, &source, &destination, &sizeObj, &destinationOffsetObj))
            return nullptr;

        render::ResourceHandle destHandle;
        bool isDestBuffer = false;
        if (!getResourceObject(moduleState, destination, destHandle, isDestBuffer))
        {
            PyErr_SetString(moduleState.exObj(), "Destination resource must be type texture or buffer");
            return nullptr;
        }

        char* bufferProtocolPtr = nullptr;
        int bufferProtocolSize = 0;
        std::vector<Py_buffer> bufferViews;
        std::vector<int> rawNums;
        bool useSrcReference = false;
        if (getBufferProtocolObject(moduleState, source, bufferProtocolPtr, bufferProtocolSize, bufferViews))
        {
            useSrcReference = true;
        }
        else
        {
            if (!getArrayOfNums(moduleState, source, rawNums))
            {
                PyErr_SetString(moduleState.exObj(), "Source buffer must be: an array of [int|float], an array.array() or any object that follows the python Buffer protocol.");
                return nullptr;
            }

            if (rawNums.empty())
            {
                PyErr_SetString(moduleState.exObj(), "Source buffer list cannot be empty");
                return nullptr;
            }

            bufferProtocolPtr = (char*)rawNums.data();
            bufferProtocolSize = (int)rawNums.size() * sizeof(int);
        }

        if (bufferProtocolPtr == nullptr || bufferProtocolSize == 0)
        {
            PyErr_SetString(moduleState.exObj(), "Source buffer must not be of size 0 or null.");
            return nullptr;
        }

        if (isDestBuffer)
        {
            int destinationOffset = 0;  

            if (destinationOffsetObj != nullptr)
            {
                if (!PyLong_Check(destinationOffsetObj))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for destination_offset must be an integer when performing buffer upload.");
                    return nullptr;
                }
    
                destinationOffset = (int)PyLong_AsLong(destinationOffsetObj);
            }

            render::UploadCommand cmd;
            cmd.setData(bufferProtocolPtr, bufferProtocolSize, destHandle);
            cmd.setBufferDestOffset(destinationOffset);
            cmdList.cmdList->writeCommand(cmd);
        }
        else
        {
            int sizeArgs[3] = { 1, 1, 1 };
            if (sizeObj != nullptr)
            {
                if (!getTupleValues(sizeObj, sizeArgs, 1, 3))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for size must be a tuple of x, y and z texture sizes for the source.");
                    return nullptr;
                }
            }

            int destinationArgs[4] = { 0, 0, 0, 0 };
            if (destinationOffsetObj != nullptr)
            {
                if (!getTupleValues(destinationOffsetObj, destinationArgs, 1, 4))
                {
                    PyErr_SetString(moduleState.exObj(), "The argument provided for desintation offset must be a tuple with 4 values, destination x, y z and mip level");
                    return nullptr;
                }
            }

            render::UploadCommand cmd;
            cmd.setData(bufferProtocolPtr, bufferProtocolSize, destHandle);
            cmd.setTextureDestInfo(sizeArgs[0], sizeArgs[1], sizeArgs[2],
                destinationArgs[0], destinationArgs[1], destinationArgs[2], destinationArgs[3]);
            cmdList.cmdList->writeCommand(cmd);
        }

        if (useSrcReference)
        {
            Py_INCREF(source);
            cmdList.references.objects.push_back(source);
        }
        Py_INCREF(destination);
        cmdList.references.objects.push_back(destination);

        Py_RETURN_NONE;
    }

    PyObject* cmdCopyAppendConsumeCounter(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        auto& cmdList = *((CommandList*)self); 
        static char* arguments[] = { "source", "destination", "destination_offset", nullptr };
        PyObject* source = nullptr;
        PyObject* destination = nullptr;
        int destinationOffset = 0;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "OO|i", arguments, &source, &destination, &destinationOffset))
            return nullptr;

        render::ResourceHandle srcHandle;
        bool isSourceBuffer = false;
        if (!getResourceObject(moduleState, source, srcHandle, isSourceBuffer) || !isSourceBuffer)
        {
            PyErr_SetString(moduleState.exObj(), "Source resource must be type buffer");
            return nullptr;
        }

        render::ResourceHandle dstHandle;
        bool isDstBuffer = false;
        if (!getResourceObject(moduleState, destination, dstHandle, isDstBuffer) || !isDstBuffer)
        {
            PyErr_SetString(moduleState.exObj(), "Destination resource must be type buffer");
            return nullptr;
        }

        Buffer* bufferObj = (Buffer*)source;
        if (!bufferObj->isAppendConsume)
        {
            PyErr_SetString(moduleState.exObj(), "Source buffer must be created with propety is_append_consume to be able to call clear counter.");
            return nullptr;
        }

        Py_INCREF(source);
        cmdList.references.objects.push_back(source);
        Py_INCREF(destination);
        cmdList.references.objects.push_back(destination);

        render::CopyAppendConsumeCounterCommand cmd;
        cmd.setData(
            render::Buffer { srcHandle.handleId },
            render::Buffer { dstHandle.handleId }, destinationOffset);
        cmdList.cmdList->writeCommand(cmd);

        Py_RETURN_NONE;
    }

    PyObject* cmdClearAppendConsume(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        auto& cmdList = *((CommandList*)self); 
        static char* arguments[] = { "source", "clear_value", nullptr };
        PyObject* source = nullptr;
        int clearValue = 0;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "O|i", arguments, &source, &clearValue))
            return nullptr;

        render::ResourceHandle srcHandle;
        bool isSourceBuffer = false;
        if (!getResourceObject(moduleState, source, srcHandle, isSourceBuffer) || !isSourceBuffer)
        {
            PyErr_SetString(moduleState.exObj(), "Source resource must be type buffer");
            return nullptr;
        }

        Buffer* bufferObj = (Buffer*)source;
        if (!bufferObj->isAppendConsume)
        {
            PyErr_SetString(moduleState.exObj(), "Source buffer must be created with propety is_append_consume to be able to call clear counter.");
            return nullptr;
        }

        Py_INCREF(source);
        cmdList.references.objects.push_back(source);

        render::ClearAppendConsumeCounter clearCmd;
        clearCmd.setData(srcHandle, clearValue);
        cmdList.cmdList->writeCommand(clearCmd);

        Py_RETURN_NONE;
    }

    PyObject* cmdBeginMarker(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        auto& cmdList = *((CommandList*)self); 
        static char* arguments[] = { "name", nullptr };
        const char* name  = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", arguments, &name))
            return nullptr;

        cmdList.cmdList->beginMarker(name);
        Py_RETURN_NONE;
    }

    PyObject* cmdEndMarker(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        auto& cmdList = *((CommandList*)self); 
        cmdList.cmdList->endMarker();
        Py_RETURN_NONE;
    }
}

}
}
