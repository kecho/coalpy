#include "CommandList.h"
#include "ModuleState.h"
#include "Shader.h"
#include "CoalpyTypeObject.h"
#include "HelperMacros.h"
#include "Resources.h"
#include "TypeIds.h"
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
            input_tables (optional): a single InResourceTable object, or an array of InResourceTable objects. If a single
                                   object is used, the table will be automatically bound to register space 0, and each resource accessed either
                                   by bindless dynamic indexing, or a hard register(t#). If an array of InResourceTable is passed, each resource
                                   table will be bound using a register space index corresponding to the table index in the array, and the rules
                                   to reference each resource within the table stay the same.
            output_tables (optional): a single OutResourceTable object, or an array of OutResourceTable objects. Same rules as input_tables apply,
                                     but we use registers u# to address the UAVs
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

    for (auto* r : pycmdList.references)
        Py_DECREF(r);

    pycmdList.~CommandList();
    Py_TYPE(self)->tp_free(self);
}

static bool getListOfBuffers(
    ModuleState& moduleState,
    PyObject* opaqueList,
    std::vector<render::Buffer>& bufferList,
    std::vector<PyObject*>& references)
{
    PyTypeObject* bufferType = moduleState.getType(TypeId::Buffer);
    if (!PyList_Check(opaqueList))
    {
        if (opaqueList->ob_type == bufferType)
        {
            Buffer& buff = *((Buffer*)opaqueList);
            bufferList.push_back(buff.buffer);
            references.push_back(opaqueList);
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
        references.push_back(obj);
    }

    return true;
}

template<typename PyTableType, typename TableType>
static bool getListOfTables(
    ModuleState& moduleState,
    PyObject* opaqueList,
    std::vector<TableType>& bufferList,
    std::vector<PyObject*>& references)
{
    PyTypeObject* pyObjType = moduleState.getType(PyTableType::s_typeId);
    if (!PyList_Check(opaqueList))
    {
        if (opaqueList->ob_type == pyObjType)
        {
            PyTableType& tobj = *((PyTableType*)opaqueList);
            bufferList.push_back(tobj.table);
            references.push_back(opaqueList);
            return true;
        }
        return false;
    }

    auto& listObj = *((PyListObject*)opaqueList);
    int listSize = Py_SIZE(opaqueList);
    for (int i = 0; i < listSize; ++i)
    {
        PyObject* obj = listObj.ob_item[i];
        if (obj->ob_type != pyObjType)
            return false;

        PyTableType& tobj = *((PyTableType*)obj);
        bufferList.push_back(tobj.table);
        references.push_back(obj);
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

static bool getArrayOfNums(ModuleState& moduleState, PyObject* constants, std::vector<int>& rawNums)
{
    if (!PyList_Check(constants))
        return false;

    auto& listObj = *((PyListObject*)constants);
    int listSize = Py_SIZE(constants);
    rawNums.reserve(listSize);
    for (int i = 0; i < listSize; ++i)
    {
        PyObject* obj = listObj.ob_item[i];
        if (PyLong_Check(obj))
        {
            rawNums.push_back((int)PyLong_AsLong(obj));
        }
        else if (PyFloat_Check(obj))
        {
            float f = (float)(PyFloat_AS_DOUBLE(obj));
            rawNums.push_back(*reinterpret_cast<int*>((&f)));
        }
        else
            return false;
    }

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
        static char* arguments[] = { "x", "y", "z", "shader", "name", "constants", "input_tables", "output_tables", nullptr };
        int x = 1;
        int y = 1;
        int z = 1;
        const char* name = nullptr;
        PyObject* shader = nullptr;
        PyObject* constants = nullptr;
        PyObject* input_tables = nullptr;
        PyObject* output_tables = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "iiiO|sOOO", arguments, &x, &y, &z, &shader, &name, &constants, &input_tables, &output_tables))
            return nullptr;
        
        if (x <= 0 || y <= 0 || z <= 0)
        {
            PyErr_SetString(moduleState.exObj(), "x, y and z arguments of dispatch must be greater or equal to 1");
            return nullptr;
        }

        auto& cmdList = *(CommandList*)self;

        render::ComputeCommand cmd;
        cmd.setDispatch(name ? name : "", x, y, z);

        //for constants / inline constants. Must be in this scope, because these pointers are needed when writeCommand is called / flushed.
        std::vector<Py_buffer> bufferViews;
        std::vector<int> rawNums;
        std::vector<render::Buffer> bufferList;
        std::vector<render::InResourceTable> inTables;
        std::vector<render::OutResourceTable> outTables;
        std::vector<PyObject*> references;

        PyTypeObject* shaderType = moduleState.getType(Shader::s_typeId);
        if (shader->ob_type != shaderType)
        {
            PyErr_SetString(moduleState.exObj(), "shader parameter must be of type coalpy.gpu.Shader");
            return nullptr;
        }

        Shader& shaderObj = *((Shader*)shader);
        references.push_back(shader);
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

        if (input_tables)
        {
            if (!getListOfTables<InResourceTable, render::InResourceTable>(moduleState, input_tables, inTables, references))
            {
                PyErr_SetString(moduleState.exObj(), "input_tables argument must be a list of InResourceTable");
                return nullptr;
            }

            if (!inTables.empty())
            {
                cmd.setInResources(inTables.data(), (int)inTables.size());
            }
        }

        if (output_tables)
        {
            if (!getListOfTables<OutResourceTable, render::OutResourceTable>(moduleState, output_tables, outTables, references))
            {
                PyErr_SetString(moduleState.exObj(), "output_tables argument must be a list of OutResourceTable");
                return nullptr;
            }

            if (!outTables.empty())
            {
                cmd.setOutResources(outTables.data(), (int)outTables.size());
            }
        }

        cmdList.cmdList->writeCommand(cmd);
        cmdList.references.insert(cmdList.references.end(), references.begin(), references.end());
        for (auto* r : references)
            Py_INCREF(r);

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
        cmdList.references.push_back(source);
        Py_INCREF(destination);
        cmdList.references.push_back(destination);
        
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
            cmdList.references.push_back(source);
        }
        Py_INCREF(destination);
        cmdList.references.push_back(destination);

        Py_RETURN_NONE;
    }

    PyObject* cmdDownloadResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        Py_RETURN_NONE;
    }
}

}
}
