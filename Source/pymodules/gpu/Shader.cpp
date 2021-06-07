#include "Shader.h"
#include "CoalpyTypeObject.h"

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
    auto& shader = *(Shader*)self;
    ModuleState& moduleState = parentModule(self);
    shader.db = &(moduleState.db());
    shader.handle = ShaderHandle();
    return 0;
}

void Shader::destroy(PyObject* self)
{
    Py_TYPE(self)->tp_free(self);
}

}
}
