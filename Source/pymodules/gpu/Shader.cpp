#include "Shader.h"
#include "HelperMacros.h"
#include "CoalpyTypeObject.h"
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <string>
#include <iostream>

namespace coalpy
{
namespace gpu
{

namespace methods
{
    static PyObject* resolve(PyObject* self, PyObject* vargs);
}

static PyMethodDef g_shaderMethods[] = {
    VA_FN(resolve, resolve, "Waits for shader compilation to be finished. Resolve is mostly implicitely called upon any operation requiring the shader's definition."),
    FN_END
};

void Shader::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Shader";
    t.tp_basicsize = sizeof(Shader);
    t.tp_doc   = "Class that represents a shader.\n"
                 "Constructor Arguments:\n"
                 "file: text file with shader code.\n"
                 "name (optional): identifier of the shader to use. Default will be the file name.\n"
                 "main_function (optional): entry point of shader. Default is 'main'.\n"
                 "NOTE: to create an inline shader, use the function gpu.inlineShader()";
    
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Shader::init;
    t.tp_dealloc = Shader::destroy;
    t.tp_methods = g_shaderMethods;
}

int Shader::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    const char* shaderName = nullptr;
    const char* shaderFile = nullptr;
    const char* mainFunction = "main";

    static char* argnames[] = { "file", "name", "main_function", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|ss", argnames, &shaderFile, &shaderName, &mainFunction))
        return -1;

    std::string sshaderName = shaderName ? shaderName : "";
    if (sshaderName == "")
    {
        std::string filePath = shaderFile;
        FileUtils::getFileName(filePath, sshaderName);
    }

    auto& shader = *(Shader*)self;

    ShaderDesc desc;
    desc.type = ShaderType::Compute;
    desc.name = sshaderName.c_str();
    desc.mainFn = mainFunction;
    desc.path = shaderFile;

    shader.db = &(moduleState.db());
    shader.handle = shader.db->requestCompile(desc);
    return 0;
}

void Shader::destroy(PyObject* self)
{
    Py_TYPE(self)->tp_free(self);
}

namespace methods
{
    static PyObject* resolve(PyObject* self, PyObject* vargs)
    {
        auto* shader = (Shader*)self;
        shader->db->resolve(shader->handle);
        Py_RETURN_NONE;
    }
}

}
}
