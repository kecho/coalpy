#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>

struct CoalPyModuleState
{
    const char* d = nullptr;
};

namespace coalpy
{

static PyObject*
Test(PyObject* self, PyObject* args)
{
    const char* command;
    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;

    auto state = (CoalPyModuleState*)PyModule_GetState(self);
    std::cout << state->d << std::endl;
    
    return PyLong_FromLong(99);
}

}

static PyMethodDef s_CoalPyMethods[] = {
    {"test",  coalpy::Test, METH_VARARGS,
     "Test function."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef s_CoalPyModuleDef = {
    PyModuleDef_HEAD_INIT,
    "gpu",   /* name of module */
    NULL, /* module documentation, may be NULL */
    sizeof(CoalPyModuleState),       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    s_CoalPyMethods
};

PyMODINIT_FUNC
PyInit_gpu(void)
{
    std::cout << "PY TEST MODULE BITCHES " << std::endl;
    PyObject* moduleObj = PyModule_Create(&s_CoalPyModuleDef);
    auto state = (CoalPyModuleState*)PyModule_GetState(moduleObj);
    state->d = "hello world";
    return moduleObj;
}
