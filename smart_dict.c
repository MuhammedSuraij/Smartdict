#include <Python.h>

static PyObject* say_hello(PyObject* self,PyObject* args)
{
    return PyUnicode_FromString("Hello from smart dict");
}

static PyMethodDef SmartDictMethods[]={
    {"say_hello",say_hello,METH_NOARGS,"Say hello from c"},
    {NULL,NULL,0,NULL}
};

static struct PyModuleDef smartdictmodule={
    PyModuleDef_HEAD_INIT,
    "smart_dict",
    "SmartDict C Extension",
    -1,
    SmartDictMethods
};

PyMODINIT_FUNC PyInit_smart_dict(void)
{
    return PyModule_Create(&smartdictmodule);
}