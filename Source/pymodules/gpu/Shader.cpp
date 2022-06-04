#include "Shader.h"
#include "HelperMacros.h"
#include "CoalpyTypeObject.h"
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.core/String.h>
#include <string>
#include <iostream>

namespace coalpy
{
namespace gpu
{

namespace methods
{
    #include "bindings/MethodDecl.h"
    #include "bindings/Shader.inl"
}

static PyMethodDef g_shaderMethods[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/Shader.inl"
    FN_END
};

void Shader::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Shader";
    t.tp_basicsize = sizeof(Shader);
    t.tp_doc   = R"(
    Class that represents a shader.

    Constructor:
        file (str): text file with shader code.
        name (str)(optional): identifier of the shader to use. Default will be the file name.
        main_function (str)(optional): entry point of shader. Default is 'main'.
        defines (array of str): an array of strings with defines for the shader. Utilize the = sign to set the definition inside the shader. I.e. ["HDR_LIGHTING=1"] will create the define HDR_LIGHTING inside the shader to a value of 1
        source_code (str): text file with the source. If source is set, file will be ignored and the shader will be created from source.
    )";

    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Shader::init;
    t.tp_dealloc = Shader::destroy;
    t.tp_methods = g_shaderMethods;
}

static bool parseStringList(ModuleState& state, PyObject* pyObj, std::vector<std::string>& outList)
{
    if (pyObj == nullptr || !PyList_Check(pyObj))
    {
        PyErr_SetString(state.exObj(), "Expected list of strings.");
        return false;
    }

    Py_ssize_t listSize = (Py_ssize_t)PyList_Size(pyObj);
    auto& listObject = *((PyListObject*)pyObj);

    for (int i = 0; i < listSize; ++i)
    {
        PyObject* element = listObject.ob_item[i];
        if (element == nullptr)
            continue;
    
        if (!PyUnicode_Check(element))
        {
            PyErr_SetString(state.exObj(), "Expected a string for element inside list.");
            return false;
        }

        //ooff is there a faster way?
        //std::wstring wstr = (wchar_t*)PyUnicode_DATA(element);
        Py_ssize_t strLen;
        wchar_t* wstrRaw = PyUnicode_AsWideCharString(element, &strLen);
        std::wstring wstr = wstrRaw;
        if (wstrRaw)
            PyMem_Free(wstrRaw);
        outList.push_back(ws2s(wstr));
    }

    return true;
}

int Shader::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto& shader = *(Shader*)self;
    shader.handle = ShaderHandle();

    ModuleState& moduleState = parentModule(self);
    if (!moduleState.checkValidDevice())
        return -1;

    const char* shaderName = nullptr;
    const char* shaderFile = nullptr;
    const char* mainFunction = "main";
    const char* sourceCode = nullptr;
    PyObject* defineList = nullptr;
    

    static char* argnames[] = { "file", "name", "main_function", "defines", "source_code", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|sssOs", argnames, &shaderFile, &shaderName, &mainFunction, &defineList, &sourceCode))
        return -1;

    std::string sshaderName = shaderName ? shaderName : "";
    std::vector<std::string> defineStrList;
    if (defineList != nullptr)
    {
        if (!parseStringList(moduleState, defineList, defineStrList))
            return -1;
    }

    if (sourceCode == nullptr)
    {
        if (shaderFile == nullptr)
        {
            PyErr_SetString(moduleState.exObj(), "Cant create a shader: no parameter file or source_code specified.");
            return -1;
        }
            
        if (sshaderName == "")
        {
            std::string filePath = shaderFile;
            FileUtils::getFileName(filePath, sshaderName);
        }

        ShaderDesc desc;
        desc.type = ShaderType::Compute;
        desc.name = sshaderName.c_str();
        desc.mainFn = mainFunction;
        desc.path = shaderFile;
        desc.defines = std::move(defineStrList);
        shader.handle = moduleState.db().requestCompile(desc);
    }
    else
    {
        ShaderInlineDesc desc;
        desc.type = ShaderType::Compute;
        desc.mainFn = mainFunction;
        desc.name = sshaderName.c_str();
        desc.immCode = sourceCode;
        desc.defines = std::move(defineStrList);
        shader.handle = moduleState.db().requestCompile(desc);
    }

    shader.db = &(moduleState.db());
    return 0;
}

void Shader::destroy(PyObject* self)
{
    Shader* shaderObj = (Shader*)self;
    shaderObj->~Shader();
    Py_TYPE(self)->tp_free(self);
}

namespace methods
{
    static PyObject* resolve(PyObject* self, PyObject* kwds, PyObject* vargs)
    {
        auto* shader = (Shader*)self;
        shader->db->resolve(shader->handle);
        Py_RETURN_NONE;
    }

    static PyObject* isValid(PyObject* self, PyObject* kwds, PyObject* vargs)
    {
        auto* shader = (Shader*)self;
        if (shader->db->isValid(shader->handle))
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }
}

}
}
