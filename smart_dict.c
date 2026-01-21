#include <Python.h>

/* =========================
   SmartDict Object
   ========================= */

typedef struct {
    PyObject_HEAD
    PyObject *dict;   /* key -> list of versions */
} SmartDictObject;

/* =========================
   tp_new
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
   tp_init
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
            PyObject *list = PyList_New(1);
            if (!list)
                return -1;

            Py_INCREF(value);
            PyList_SET_ITEM(list, 0, value);

            if (PyDict_SetItem(self->dict, key, list) < 0) {
                Py_DECREF(list);
                return -1;
            }
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
   d[key] = value / del d[key]
   ========================= */

static int
SmartDict_setitem(PyObject *self, PyObject *key, PyObject *value)
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *list;

    if (value == NULL)
        return PyDict_DelItem(sd->dict, key);

    list = PyDict_GetItemWithError(sd->dict, key);
    if (!list) {
        if (PyErr_Occurred())
            return -1;

        list = PyList_New(0);
        if (!list)
            return -1;

        if (PyDict_SetItem(sd->dict, key, list) < 0) {
            Py_DECREF(list);
            return -1;
        }
        Py_DECREF(list);
        list = PyDict_GetItem(sd->dict, key);
    }

    return PyList_Append(list, value);
}

/* =========================
   d[key]  (latest)
   ========================= */

static PyObject *
SmartDict_getitem(PyObject *self, PyObject *key)
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *list = PyDict_GetItem(sd->dict, key);

    if (!list) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }

    Py_ssize_t size = PyList_Size(list);
    if (size <= 0) {
        PyErr_SetObject(PyExc_KeyError, key);
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

    if (!PyArg_ParseTuple(args, "Oi", &key, &version))
        return NULL;

    PyObject *list = PyDict_GetItem(sd->dict, key);
    if (!list) {
        PyErr_SetObject(PyExc_KeyError, key);
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

    if (!PyArg_ParseTuple(args, "O|i", &key, &version))
        return NULL;

    PyObject *list = PyDict_GetItem(sd->dict, key);
    if (!list) {
        PyErr_SetObject(PyExc_KeyError, key);
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

    if (PySequence_DelItem(list, version - 1) < 0)
        return NULL;

    if (PyList_Size(list) == 0)
        PyDict_DelItem(sd->dict, key);

    Py_RETURN_NONE;
}

/* =========================
   undo(key [, steps])
   ========================= */

static PyObject *
SmartDict_undo(PyObject *self, PyObject *args)
{
    PyObject *key;
    int steps = 1;
    SmartDictObject *sd = (SmartDictObject *)self;

    if (!PyArg_ParseTuple(args, "O|i", &key, &steps))
        return NULL;

    PyObject *list = PyDict_GetItem(sd->dict, key);
    if (!list) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }

    Py_ssize_t size = PyList_Size(list);
    if (steps <= 0 || steps > size) {
        PyErr_SetString(PyExc_IndexError, "Invalid undo steps");
        return NULL;
    }

    while (steps--) {
        if (PySequence_DelItem(list, PyList_Size(list) - 1) < 0)
            return NULL;
    }

    if (PyList_Size(list) == 0)
        PyDict_DelItem(sd->dict, key);

    Py_RETURN_NONE;
}

/* =========================
   snapshot()
   ========================= */

static PyObject *
SmartDict_snapshot(SmartDictObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *snapshot = PyDict_New();
    if (!snapshot)
        return NULL;

    PyObject *key, *list;
    Py_ssize_t pos = 0;

    while (PyDict_Next(self->dict, &pos, &key, &list)) {
        Py_ssize_t size = PyList_Size(list);
        if (size > 0) {
            PyObject *value = PyList_GetItem(list, size - 1); // borrowed
            Py_INCREF(value);                                 // snapshot owns it
            if (PyDict_SetItem(snapshot, key, value) < 0) {
                Py_DECREF(value);
                Py_DECREF(snapshot);
                return NULL;
            }
            Py_DECREF(value);
        }
    }

    return snapshot;
}


/* =========================
   keys / values / items
   ========================= */

static PyObject *
SmartDict_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyDict_Keys(((SmartDictObject *)self)->dict);
}

static PyObject *
SmartDict_values(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *res = PyList_New(0);
    if (!res)
        return NULL;

    PyObject *key, *list;
    Py_ssize_t pos = 0;

    while (PyDict_Next(sd->dict, &pos, &key, &list)) {
        Py_ssize_t size = PyList_Size(list);
        if (size > 0)
            PyList_Append(res, PyList_GetItem(list, size - 1));
    }
    return res;
}

static PyObject *
SmartDict_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *res = PyList_New(0);
    if (!res)
        return NULL;

    PyObject *key, *list;
    Py_ssize_t pos = 0;

    while (PyDict_Next(sd->dict, &pos, &key, &list)) {
        Py_ssize_t size = PyList_Size(list);
        if (size > 0) {
            PyObject *pair = PyTuple_Pack(2, key, PyList_GetItem(list, size - 1));
            PyList_Append(res, pair);
            Py_DECREF(pair);
        }
    }
    return res;
}

/* =========================
   repr
   ========================= */

static PyObject *
SmartDict_repr(PyObject *self)
{
    SmartDictObject *sd = (SmartDictObject *)self;
    PyObject *tmp = PyDict_New();
    if (!tmp)
        return NULL;

    PyObject *key, *list;
    Py_ssize_t pos = 0;

    while (PyDict_Next(sd->dict, &pos, &key, &list)) {
        Py_ssize_t size = PyList_Size(list);
        if (size > 0) {
            PyObject *val = PyList_GetItem(list, size - 1);
            Py_INCREF(val);
            PyDict_SetItem(tmp, key, val);
            Py_DECREF(val);
        }
    }

    PyObject *r = PyObject_Repr(tmp);
    Py_DECREF(tmp);
    return r;
}

/* =========================
   len(d)
   ========================= */

static Py_ssize_t
SmartDict_length(PyObject *self)
{
    return PyDict_Size(((SmartDictObject *)self)->dict);
}

/* =========================
   iteration
   ========================= */

static PyObject *
SmartDict_iter(PyObject *self)
{
    return PyObject_GetIter(((SmartDictObject *)self)->dict);
}

/* =========================
   Mapping
   ========================= */

static PyMappingMethods SmartDict_as_mapping = {
    .mp_length = SmartDict_length,
    .mp_subscript = SmartDict_getitem,
    .mp_ass_subscript = SmartDict_setitem
};

/* =========================
   Methods table
   ========================= */

static PyMethodDef SmartDict_methods[] = {
    {"get", SmartDict_get, METH_VARARGS, "Get version"},
    {"delete", SmartDict_delete, METH_VARARGS, "Delete key or version"},
    {"undo", SmartDict_undo, METH_VARARGS, "Undo last versions"},
    {"snapshot", (PyCFunction)SmartDict_snapshot, METH_NOARGS, "Create snapshot"},
    {"keys", SmartDict_keys, METH_NOARGS, "Keys"},
    {"values", SmartDict_values, METH_NOARGS, "Latest values"},
    {"items", SmartDict_items, METH_NOARGS, "Latest items"},
    {NULL}
};

/* =========================
   Type
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
    .tp_iter = SmartDict_iter,
    .tp_repr = SmartDict_repr,
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
    if (PyType_Ready(&SmartDictType) < 0)
        return NULL;

    PyObject *m = PyModule_Create(&smartdictmodule);
    if (!m)
        return NULL;

    Py_INCREF(&SmartDictType);
    PyModule_AddObject(m, "SmartDict", (PyObject *)&SmartDictType);
    return m;
}
