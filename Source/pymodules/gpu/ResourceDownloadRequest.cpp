#include "ResourceDownloadRequest.h"
#include "ModuleState.h"
#include "Resources.h"
#include "HelperMacros.h"
#include "CoalpyTypeObject.h"
#include <coalpy.render/CommandList.h>
#include <coalpy.render/IDevice.h>

namespace coalpy
{
namespace gpu
{

namespace methods
{
    #include "bindings/MethodDecl.h"
    #include "bindings/ResourceDownloadRequest.inl"
}

static PyMethodDef g_resourceDownloadMethods[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/ResourceDownloadRequest.inl"
    FN_END
};

void ResourceDownloadRequest::constructType(CoalpyTypeObject& o)
{
    auto& t = o.pyObj;
    t.tp_name = "gpu.ResourceDownloadRequest";
    t.tp_basicsize = sizeof(ResourceDownloadRequest);
    t.tp_doc = R"(
    Class that represents a GPU -> CPU resource download request.
    This class can be used to read the memory of a resource in the CPU synchorneously or asynchronesouly (throug polling).

    Constructor:
        resource (Texture or Buffer): The resource to bind and read from the GPU.
        slice_index : If the resource is a texture array, gets the slice index. If out of range or not a texture array, an exception is thrown. Default value is 0.
        mip_level : If the resource is a texture, gets the mip level. If the mip level is out of range or not a texture, an exception is thrown. Default value is 0.
    )";

    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = ResourceDownloadRequest::init;
    t.tp_dealloc = ResourceDownloadRequest::destroy;
    t.tp_methods = g_resourceDownloadMethods;
}

int ResourceDownloadRequest::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto& request = *((ResourceDownloadRequest*)self);
    new (&request) ResourceDownloadRequest;
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    static char* arguments[] = { "resource", "slice_index", "mip_level", nullptr };
    PyObject* resourcePyObj = nullptr;
    int sliceIndex = 0;
    int mipLevel = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "O|ii", arguments,
        &resourcePyObj,
        &sliceIndex,
        &mipLevel))
        return -1;

    const PyTypeObject* textureType = moduleState.getType(TypeId::Texture);
    const PyTypeObject* bufferType = moduleState.getType(TypeId::Buffer);
    
    if (resourcePyObj == nullptr || resourcePyObj == Py_None)
    {
        PyErr_SetString(moduleState.exObj(), "resource must not be none or null.");
        return -1;
    }

    const PyTypeObject* resourceType = Py_TYPE(resourcePyObj);
    bool isTexture = resourceType == textureType;
    bool isBuffer = resourceType == bufferType;

    if (!isTexture && !isBuffer)
    {
        PyErr_SetString(moduleState.exObj(), "resource must be a texture or buffer");
        return -1;
    }

    if (isBuffer && sliceIndex != 0 && mipLevel != 0)
    {
        PyErr_SetString(moduleState.exObj(), "resource of type Buffer must have a mip_level and slice_index of 0.");
        return -1;
    }

    render::ResourceHandle renderResource = isBuffer ? (render::ResourceHandle)(((Buffer*)resourcePyObj)->buffer) : (render::ResourceHandle)(((Texture*)resourcePyObj)->texture);
    if (!renderResource.valid())
    {
        PyErr_SetString(moduleState.exObj(), "resource is not valid.");
        return -1;
    }

    request.resource = renderResource;
    render::CommandList* cmdList = moduleState.newCommandList();
    render::DownloadCommand cmd;
    cmd.setData(renderResource);
    cmd.setMipLevel(mipLevel);
    cmd.setArraySlice(sliceIndex);
    cmdList->writeCommand(cmd);
    cmdList->finalize();

    render::ScheduleStatus scheduleStatus = moduleState.device().schedule(&cmdList, 1, render::ScheduleFlags_GetWorkHandle);
    if (!scheduleStatus.success())
    {
        PyErr_Format(moduleState.exObj(), "Failed creating resource download request, error: %s", scheduleStatus.message.c_str());
        return -1;
    }

    moduleState.deleteCommandList(cmdList);

    request.mipLevel = mipLevel;
    request.sliceIndex = sliceIndex;
    request.workHandle = scheduleStatus.workHandle;
    request.resourcePyObj = resourcePyObj;
    Py_INCREF(resourcePyObj);
    return 0;

}

void ResourceDownloadRequest::destroy(PyObject* self)
{
    ModuleState& moduleState = parentModule(self);

    auto& request = *((ResourceDownloadRequest*)self);
    Py_XDECREF(request.resourcePyObj);
    Py_XDECREF(request.dataAsByteArray);
    Py_XDECREF(request.rowBytesPitchObject);

    if (request.workHandle.valid())
        moduleState.device().release(request.workHandle);

    request.~ResourceDownloadRequest();
    Py_TYPE(self)->tp_free(self);
}

namespace methods
{

PyObject* resolve(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return nullptr;

    auto& request = *((ResourceDownloadRequest*)self);
    if (request.resolved)
        Py_RETURN_NONE;

    if (!request.workHandle.valid())
    {
        PyErr_SetString(moduleState.exObj(), "Cannot resolve ResourceDownloadRequest, invalid internal work handle.");
        return nullptr;
    }

    render::WaitStatus waitStatus = moduleState.device().waitOnCpu(request.workHandle, -1);
    if (!waitStatus.success())
    {
        PyErr_Format(moduleState.exObj(), "Failed resolving resource data from GPU. Internal error %s", waitStatus.message.c_str());
        return nullptr;
    }

    request.resolved = true;
    Py_RETURN_NONE;
}

PyObject* isReady(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return nullptr;

    auto& request = *((ResourceDownloadRequest*)self);
    if (request.resolved)
        Py_RETURN_TRUE;

    if (!request.workHandle.valid())
    {
        PyErr_SetString(moduleState.exObj(), "Cannot resolve ResourceDownloadRequest, invalid internal work handle.");
        return nullptr;
    }

    render::WaitStatus waitStatus = moduleState.device().waitOnCpu(request.workHandle, 0);
    if (waitStatus.type == render::WaitErrorType::Ok)
    {
        request.resolved = true;
        Py_RETURN_TRUE;
    }
    else if (waitStatus.type == render::WaitErrorType::NotReady)
        Py_RETURN_FALSE;
    else
    {
        PyErr_Format(moduleState.exObj(), "Failed checking readiness of resource data from GPU. Internal error %s", waitStatus.message.c_str());
        return nullptr;
    }
}

static bool getDataCommon(ModuleState& moduleState, ResourceDownloadRequest& request, PyObject* vargs, PyObject* kwds, render::DownloadStatus& outStatus)
{
    if (!request.resolved)
    {
        PyErr_SetString(moduleState.exObj(), "Cannot get request data. You must call resolve() which blocks the CPU or poll using isReady until isReady gets you a value of true. Otherwise getting data is not allowed.");
        return false;
    }

    render::DownloadStatus downloadStatus = moduleState.device().getDownloadStatus(request.workHandle, request.resource, request.mipLevel, request.sliceIndex);
    if (!downloadStatus.success())
    {
        PyErr_Format(moduleState.exObj(), "Error while getting GPU resource state. Ensure you are calling resolve() or isReady()");
        return false;
    }

    outStatus = downloadStatus;
    return true;
}

PyObject* dataAsByteArray(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return nullptr;

    auto& request = *((ResourceDownloadRequest*)self);
    if (request.dataAsByteArray)
    {
        Py_INCREF(request.dataAsByteArray);
        return request.dataAsByteArray;
    }

    render::DownloadStatus status;
    if (!getDataCommon(moduleState, request, vargs, kwds, status))
        return nullptr;

    request.rowBytesPitchObject = PyLong_FromLongLong((long)status.rowPitch);
    request.dataAsByteArray = PyBytes_FromStringAndSize((const char*)status.downloadPtr, status.downloadByteSize);

    Py_INCREF(request.dataAsByteArray);
    return request.dataAsByteArray;
}

PyObject* dataByteRowPitch(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    auto& request = *((ResourceDownloadRequest*)self);
    if (request.rowBytesPitchObject)
    {
        Py_INCREF(request.rowBytesPitchObject);
        return request.rowBytesPitchObject;
    }

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return nullptr;

    PyErr_SetString(moduleState.exObj(), "Data object has not been created. Before calling data_byte_row_pitch, ensure to call data_as_bytearray. Only then the pitch is accessible");
    return nullptr;
}

} //namespace methods

} //namespace gpu
} //namespace coalpy
