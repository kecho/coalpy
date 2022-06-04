//Include before declaring c++ binding methods
#define COALPY_FN(pyname, cppname, doc)\
    static PyObject* cppname(PyObject* self, PyObject* vargs, PyObject* kwds);
