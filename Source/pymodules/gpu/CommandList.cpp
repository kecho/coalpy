#include "CommandList.h"
#include "ModuleState.h"
#include "Shader.h"
#include "CoalpyTypeObject.h"
#include "HelperMacros.h"
#include "Resources.h"
#include "TypeIds.h"
#include "PyUtils.h"
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
    static PyObject* cmdDownloadResource(PyObject* self, PyObject* vargs, PyObject* kwds);
}

static PyMethodDef g_cmdListMethods[] = {
    KW_FN(dispatch, cmdDispatch,R"(
        Dispatch method, which executes a compute shader with resources bound.
        
        Parameters:
            x (int): the number of groups on the x axis.
            y (int): the number of groups on the y axis.
            z (int):  the number of groups on the z axis.
            shader (Shader): object of type Shader. This will be the compute shader launched on the GPU.
            name (str)(optional): Debug name of this dispatch to see in render doc / profiling captures.
            constants (optional): Constant can be, an array of ints and floats, an array.array
                                  or any object compatible with the object protocol, or a list of Buffer objects.
                                  If a list of Buffer objects, these can be indexed via register(b#) in the shader used,
                                  Every other type will always be bound to a constant buffer on register(b0).
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
    )"),
    KW_FN(copy_resource, cmdCopyResource, R"(
        copy_resource method, copies one resource to another.

        Parameters:
            source (Texture or Buffer): an object of type Texture or Buffer. This will be the source resource to copy from.
            destination (Texture or Buffer): an object of type Texture or Buffer. This will be the destination resource to copy to.
    )"),
    KW_FN(upload_resource, cmdUploadResource, R"(
        upload_resource method. Uploads an python array [], an array.array or any buffer protocol compatible object to a gpu resource.
        
        Parameters:
            source: an array of ints and floats, or an array.array object or any object compatible with the buffer protocol.
            destination (Texture or Buffer): a destination object of type Texture or Buffer.
    )"),
    /*KW_FN(download_resource, cmdDownloadResource, ""),*/
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

static bool getResourceObject(ModuleState& moduleState, PyObject* obj, render::ResourceHandle& handle)
{
    auto* texType = moduleState.getType(Texture::s_typeId);
    auto* buffType = moduleState.getType(Buffer::s_typeId);
    if (texType == obj->ob_type)
    {
        auto* texObj = (Texture*)obj;
        handle = texObj->texture;
        return true;
    }
    else if (buffType == obj->ob_type)
    {
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
        static char* arguments[] = { "x", "y", "z", "shader", "name", "constants", "samplers", "inputs", "outputs", nullptr };
        int x = 1;
        int y = 1;
        int z = 1;
        const char* name = nullptr;
        PyObject* shader = nullptr;
        PyObject* constants = nullptr;
        PyObject* sampler_tables = nullptr;
        PyObject* input_tables = nullptr;
        PyObject* output_tables = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "iiiO|sOOOO", arguments, &x, &y, &z, &shader, &name, &constants, &sampler_tables, &input_tables, &output_tables))
            return nullptr;
        
        if (x <= 0 || y <= 0 || z <= 0)
        {
            PyErr_SetString(moduleState.exObj(), "x, y and z arguments of dispatch must be greater or equal to 1");
            return nullptr;
        }

        CommandListReferences references;
        auto& cmdList = *(CommandList*)self;

        render::ComputeCommand cmd;
        cmd.setDispatch(name ? name : "", x, y, z);

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
        static char* arguments[] = { "source", "destination", nullptr };
        PyObject* source = nullptr;
        PyObject* destination = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "OO", arguments, &source, &destination))
            return nullptr;

        render::ResourceHandle sourceHandle;
        render::ResourceHandle destHandle;
        if (!getResourceObject(moduleState, source, sourceHandle))
        {
            PyErr_SetString(moduleState.exObj(), "Source resource must be type texture or buffer");
            return nullptr;
        }

        if (!getResourceObject(moduleState, destination, destHandle))
        {
            PyErr_SetString(moduleState.exObj(), "Destination resource must be type texture or buffer");
            return nullptr;
        }

        Py_INCREF(source);
        cmdList.references.objects.push_back(source);
        Py_INCREF(destination);
        cmdList.references.objects.push_back(destination);
        
        render::CopyCommand cmd;
        cmd.setResources(sourceHandle, destHandle);
        cmdList.cmdList->writeCommand(cmd);

        Py_RETURN_NONE;
    }

    PyObject* cmdUploadResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        auto& cmdList = *((CommandList*)self); 
        static char* arguments[] = { "source", "destination", nullptr };
        PyObject* source = nullptr;
        PyObject* destination = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "OO", arguments, &source, &destination))
            return nullptr;

        render::ResourceHandle destHandle;
        if (!getResourceObject(moduleState, destination, destHandle))
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
        
        if (useSrcReference)
        {
            Py_INCREF(source);
            cmdList.references.objects.push_back(source);
        }
        Py_INCREF(destination);
        cmdList.references.objects.push_back(destination);

        render::UploadCommand cmd;
        cmd.setBufferDestOffset(0u);
        cmd.setData(bufferProtocolPtr, bufferProtocolSize, destHandle);
        cmdList.cmdList->writeCommand(cmd);

        Py_RETURN_NONE;
    }

    PyObject* cmdDownloadResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        Py_RETURN_NONE;
    }
}

}
}
