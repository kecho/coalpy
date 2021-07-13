#include "CommandList.h"
#include "ModuleState.h"
#include "CoalpyTypeObject.h"
#include "HelperMacros.h"
#include <coalpy.render/IDevice.h>

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

    return 0;
}

void CommandList::destroy(PyObject* self)
{
    Py_TYPE(self)->tp_free(self);
}

namespace methods
{
    PyObject* cmdDispatch(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
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
