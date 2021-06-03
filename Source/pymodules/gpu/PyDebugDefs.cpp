#define PY_SSIZE_T_CLEAN

#include <Python.h>

#ifdef assert
#undef assert
#endif

#include <coalpy.core/Assert.h>

Py_ssize_t _Py_RefTotal = 0;
void _Py_NegativeRefcount(const char* filename, int lineno,
    PyObject* op)
{
    CPY_ASSERT_FMT(false, "Python Neg Refcount: %s : %d", filename, lineno);
}