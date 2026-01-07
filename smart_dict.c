#include <Python.h>

/* =========================
   SmartDict Object
   ========================= */

typedef struct {
    PyObject_HEAD
    PyObject *dict;   // key -> list of versions
} SmartDictObject;

/* =========================
   tp_new (allocation only)
   ========================= */

static PyObject *
SmartDict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SmartDictObject *self =
        (SmartDictObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->dict = PyDict_New();
    if (!self->dict) {
        Py_DECREF(self);
        return NULL;
    }

    return (PyObject *)self;
}

/* =========================
   tp_init (argument handling)
   ========================= */

static int
SmartDict_init(SmartDictObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *init_dict = NULL;

    if (!PyArg_ParseTuple(args, "|O!", &PyDict_Type, &init_dict))
        return -1;

    if (init_dict) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(init_dict, &pos, &key, &value)) {
            PyObject *list = PyList_New(0);
            if (!list)
                return -1;

            PyList_Append(list, value);
            PyDict_SetItem(self->dict, key, list);
            Py_DECREF(list);
        }
    }
    return 0;
}

/* =========================
   tp_dealloc
   ========================= */

static void
SmartDict_dealloc(SmartDictObject *self)
{
    Py_XDECREF(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/* =========================
   d[key] = value
   del d[key]
   ========================= */

static int
SmartDict_setitem(PyObject *self, PyObject *key, PyObject *value)
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *list;

    /* del d[key] */
    if (value == NULL) {
        return PyDict_DelItem(sd->dict, key);
    }

    list = PyDict_GetItem(sd->dict, key);

    if (!list) {
        list = PyList_New(0);
        if (!list)
            return -1;

        if (PyDict_SetItem(sd->dict, key, list) < 0) {
            Py_DECREF(list);
            return -1;
        }
        Py_DECREF(list);
    }

    return PyList_Append(list, value);
}

/* =========================
   d[key]  -> latest version
   ========================= */

static PyObject *
SmartDict_getitem(PyObject *self, PyObject *key)
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *list = PyDict_GetItem(sd->dict, key);

    if (!list) {
        PyErr_SetString(PyExc_KeyError, "Key not found");
        return NULL;
    }

    Py_ssize_t size = PyList_Size(list);
    if (size == 0) {
        PyErr_SetString(PyExc_KeyError, "No values stored");
        return NULL;
    }

    PyObject *value = PyList_GetItem(list, size - 1);
    Py_INCREF(value);
    return value;
}

/* =========================
   get(key, version)
   ========================= */

static PyObject *
SmartDict_get(PyObject *self, PyObject *args)
{
    PyObject *key;
    int version;
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *list;

    if (!PyArg_ParseTuple(args, "Oi", &key, &version))
        return NULL;

    list = PyDict_GetItem(sd->dict, key);
    if (!list) {
        PyErr_SetString(PyExc_KeyError, "Key not found");
        return NULL;
    }

    if (version <= 0 || version > PyList_Size(list)) {
        PyErr_SetString(PyExc_IndexError, "Invalid version");
        return NULL;
    }

    PyObject *value = PyList_GetItem(list, version - 1);
    Py_INCREF(value);
    return value;
}

/* =========================
   delete(key [, version])
   ========================= */

static PyObject *
SmartDict_delete(PyObject *self, PyObject *args)
{
    PyObject *key;
    int version = -1;
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *list;

    if (!PyArg_ParseTuple(args, "O|i", &key, &version))
        return NULL;

    list = PyDict_GetItem(sd->dict, key);
    if (!list) {
        PyErr_SetString(PyExc_KeyError, "Key not found");
        return NULL;
    }

    if (version == -1) {
        PyDict_DelItem(sd->dict, key);
        Py_RETURN_NONE;
    }

    if (version <= 0 || version > PyList_Size(list)) {
        PyErr_SetString(PyExc_IndexError, "Invalid version");
        return NULL;
    }

    PySequence_DelItem(list, version - 1);

    if (PyList_Size(list) == 0)
        PyDict_DelItem(sd->dict, key);

    Py_RETURN_NONE;
}

/* =========================
   Mapping protocol
   ========================= */

static PyMappingMethods SmartDict_as_mapping = {
    .mp_length = 0,
    .mp_subscript = SmartDict_getitem,
    .mp_ass_subscript = SmartDict_setitem
};

/* =========================
   Methods
   ========================= */

static PyMethodDef SmartDict_methods[] = {
    {"get", SmartDict_get, METH_VARARGS, "Get specific version"},
    {"delete", SmartDict_delete, METH_VARARGS, "Delete key or version"},
    {NULL}
};

/* =========================
   Type definition
   ========================= */

static PyTypeObject SmartDictType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "smart_dict.SmartDict",
    .tp_basicsize = sizeof(SmartDictObject),
    .tp_dealloc = (destructor)SmartDict_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Versioned Smart Dictionary",
    .tp_as_mapping = &SmartDict_as_mapping,
    .tp_methods = SmartDict_methods,
    .tp_new = SmartDict_new,
    .tp_init = (initproc)SmartDict_init,
};

/* =========================
   Module
   ========================= */

static PyModuleDef smartdictmodule = {
    PyModuleDef_HEAD_INIT,
    "smart_dict",
    "Smart Dictionary Module",
    -1,
};

PyMODINIT_FUNC
PyInit_smart_dict(void)
{
    PyObject *m;

    if (PyType_Ready(&SmartDictType) < 0)
        return NULL;

    m = PyModule_Create(&smartdictmodule);
    if (!m)
        return NULL;

    Py_INCREF(&SmartDictType);
    PyModule_AddObject(m, "SmartDict",
                       (PyObject *)&SmartDictType);

    return m;
}
