#include "Shader.h"
#include "CoalpyTypeObject.h"
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <string>

namespace coalpy
{
namespace gpu
{

void Shader::constructType(PyTypeObject& t)
{
    t.tp_name = "gpu.Shader";
    t.tp_basicsize = sizeof(Shader);
    t.tp_doc   = "Class that represnts a shader.\n"
                 "Constructor Arguments: todo\n";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = Shader::init;
    t.tp_dealloc = Shader::destroy;
}

int Shader::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    const char* shaderName = nullptr;
    const char* shaderFile = nullptr;
    const char* mainFunction = "";

    static char* argnames[] = { "name", "file", "mainFunction", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|ss", argnames, &shaderFile, &mainFunction, &shaderName))
    {
        return -1;
    }

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

}
}
