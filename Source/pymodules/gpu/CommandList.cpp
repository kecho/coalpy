#include "CommandList.h"
#include "ModuleState.h"
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
    KW_FN(dispatch, cmdDispatch, ""),
    KW_FN(copy_resource, cmdCopyResource, ""),
    KW_FN(upload_resource, cmdUploadResource, ""),
    KW_FN(download_resource, cmdDownloadResource, ""),
    FN_END
};

void CommandList::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.CommandList";
    t.tp_basicsize = sizeof(CommandList);
    t.tp_doc   = "";
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

    Py_TYPE(self)->tp_free(self);
}

static bool getListOfBuffers(
    ModuleState& moduleState,
    PyObject* opaqueList,
    std::vector<render::Buffer>& bufferList,
    std::vector<PyObject*> references)
{
    if (!PyList_Check(opaqueList))
        return false;

    auto& listObj = *((PyListObject*)opaqueList);
    int listSize = Py_SIZE(opaqueList);
    PyTypeObject* bufferType = moduleState.getType(TypeId::Buffer);
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
    std::vector<PyObject*> references)
{
    if (!PyList_Check(opaqueList))
        return false;

    auto& listObj = *((PyListObject*)opaqueList);
    int listSize = Py_SIZE(opaqueList);
    PyTypeObject* pyObjType = moduleState.getType(PyTableType::s_typeId);
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

namespace methods
{
    PyObject* cmdDispatch(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        static char* arguments[] = { "x", "y", "z", "name", "constants", "input_tables", "output_tables", nullptr };
        int x = 1;
        int y = 1;
        int z = 1;
        const char* name = nullptr;
        PyObject* constants = nullptr;
        PyObject* input_tables = nullptr;
        PyObject* output_tables = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "iii|sOOO", arguments, &x, &y, &z, &name, &constants, &input_tables, &output_tables))
            return nullptr;
        
        if (x <= 0 || y <= 0 || z <= 0)
        {
            PyErr_SetString(moduleState.exObj(), "x, y and z arguments of dispatch must be greater or equal to 1");
            return nullptr;
        }

        auto& cmdList = *(CommandList*)self;

        render::ComputeCommand cmd;
        cmd.setDispatch(name ? name : "", x, y, z);

        if (constants)
        {
            std::vector<render::Buffer> bufferList;
            if (getListOfBuffers(moduleState, constants, bufferList, cmdList.references))
            {
                if (!bufferList.empty())
                    cmd.setConstants(bufferList.data(), bufferList.size());
            }
        }

        if (input_tables)
        {
            std::vector<render::InResourceTable> tables;
            if (!getListOfTables<InResourceTable, render::InResourceTable>(moduleState, input_tables, tables, cmdList.references))
            {
                PyErr_SetString(moduleState.exObj(), "input_tables argument must be a list of InResourceTable");
                return nullptr;
            }

            if (!tables.empty())
            {
                cmd.setInResources(tables.data(), (int)tables.size());
            }
        }

        if (output_tables)
        {
            std::vector<render::OutResourceTable> tables;
            if (!getListOfTables<OutResourceTable, render::OutResourceTable>(moduleState, output_tables, tables, cmdList.references))
            {
                PyErr_SetString(moduleState.exObj(), "output_tables argument must be a list of OutResourceTable");
                return nullptr;
            }

            if (!tables.empty())
            {
                cmd.setOutResources(tables.data(), (int)tables.size());
            }
        }

        cmdList.cmdList->writeCommand(cmd);

        for (auto* r : cmdList.references)
            Py_INCREF(r);

        Py_RETURN_NONE;
    }

    PyObject* cmdCopyResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        Py_RETURN_NONE;
    }

    PyObject* cmdUploadResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        Py_RETURN_NONE;
    }

    PyObject* cmdDownloadResource(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        Py_RETURN_NONE;
    }
}

}
}
