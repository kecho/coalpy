#include "TinyObjLoader.h"
#include "HelperMacros.h"
#include "CoalpyTypeObject.h"

#include <string>
#include <structmember.h>
#include <tiny_obj_loader.h>

namespace coalpy
{
namespace gpu
{

template <typename T>
static int defaultInit(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    T* obj = (T*)self;
    new (obj) T;
    return 0;
}

template<typename T>
static void defaultDestroy(PyObject* self)
{
    T* obj = (T*)self;
    obj->~T();
    Py_TYPE(self)->tp_free(self);
}

/// ObjReaderConfig ///
static PyTypeObject g_objReaderConfigType = { PyVarObject_HEAD_INIT(NULL, 0) };
struct ObjReaderConfig
{
    PyObject_HEAD
    tinyobj::ObjReaderConfig obj;    
};
static PyMemberDef g_objReaderConfigMembers [] = {
    { "triangulate", T_BOOL, offsetof(ObjReaderConfig, obj) + offsetof(tinyobj::ObjReaderConfig, triangulate),  0, "" },
    { nullptr }
};
//////////////////////

/// ObjReader ///
static PyTypeObject g_objReaderType = { PyVarObject_HEAD_INIT(NULL, 0) };
struct ObjReader
{
    PyObject_HEAD
    tinyobj::ObjReader obj;    
};
namespace methods 
{
    #include "bindings/MethodDecl.h"
    #include "bindings/ObjReader.inl"
}
static PyMethodDef g_objReaderMethods[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/ObjReader.inl"
    FN_END
};
//////////////////////

void TinyObjLoader::constructType(CoalpyTypeObject& o)
{
    auto& t = o.pyObj;
    t.tp_name = "gpu.TinyObjLoader";
    t.tp_basicsize = sizeof(TinyObjLoader);
    t.tp_doc   = R"(
    )";

    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = TinyObjLoader::init;

    /// ObjReaderConfig ///
    {
        g_objReaderConfigType.tp_name = "gpu.TinyObjLoader.ObjReaderConfig";
        g_objReaderConfigType.tp_basicsize = sizeof(ObjReaderConfig);
        g_objReaderConfigType.tp_flags = Py_TPFLAGS_DEFAULT;
        g_objReaderConfigType.tp_new = PyType_GenericNew;
        g_objReaderConfigType.tp_members = g_objReaderConfigMembers;
        g_objReaderConfigType.tp_init = defaultInit<ObjReaderConfig>;
        g_objReaderConfigType.tp_dealloc = defaultDestroy<ObjReaderConfig>;
        o.children.push_back(ChildTypeObject { "ObjReaderConfig", &g_objReaderConfigType });
    }

    /// ObjReader ///
    {
        g_objReaderType.tp_name = "gpu.TinyObjLoader.ObjReader";
        g_objReaderType.tp_basicsize = sizeof(ObjReader);
        g_objReaderType.tp_flags = Py_TPFLAGS_DEFAULT;
        g_objReaderType.tp_new = PyType_GenericNew;
        g_objReaderType.tp_init = defaultInit<ObjReader>;
        g_objReaderType.tp_dealloc = defaultDestroy<ObjReader>;
        g_objReaderType.tp_methods = g_objReaderMethods;
        o.children.push_back(ChildTypeObject { "ObjReader", &g_objReaderType });
    }
}

int TinyObjLoader::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    PyErr_SetString(moduleState.exObj(), "Cannot instantiate the type of TinyObjLoader. Please use the internal types.");
    return -1;
}

namespace methods
{
    PyObject* ParseFromFile(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        auto* objReader = (ObjReader*)self;
        char* filename = nullptr;
        PyObject* optionObj = nullptr;
        static char* arguments[] = { "filename", "option", nullptr };
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|O", arguments, &filename, &optionObj))
            return nullptr;

        if (optionObj != nullptr && Py_TYPE(optionObj) != &g_objReaderConfigType)
            return nullptr;

        tinyobj::ObjReaderConfig config;
    
        if (optionObj != nullptr)
            config = ((ObjReaderConfig*)optionObj)->obj;

        std::string filenameStr = filename;
        if (objReader->obj.ParseFromFile(filenameStr, config))
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

}

}
}
