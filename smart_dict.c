#include <Python.h>

typedef struct {
    PyObject_HEAD
    PyObject *dict;
}SmartDictObject;

static PyObject* SmartDict_new(PyTypeObject *type,PyObject *args,PyObject *kwds)
{
    SmartDictObject *self;
    self = (SmartDictObject *)type->tp_alloc(type,0);
    if (self !=NULL)
    {
        self->dict=PyDict_New();
        if(!self->dict)
        {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject *)self;
}

static void SmartDict_dealloc(SmartDictObject *self)
{
    Py_XDECREF(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int SmartDict_setitem(PyObject *self,
                             PyObject *key,
                             PyObject *value)
{
    SmartDictObject *sd=(SmartDictObject *)self;
    if(value== NULL)
    {
        PyErr_SetString(PyExc_TypeError,"Cannot delete items");
        return -1;
    }
    return PyDict_SetItem(sd->dict,key,value);
}

static PyObject* SmartDict_getitem(PyObject *self,PyObject *key)
{
    SmartDictObject *sd=(SmartDictObject *)self;
    PyObject *value=PyDict_GetItem(sd->dict,key);
    if (!value)
    {
        PyErr_SetString(PyExc_KeyError,"Key Not Found");
        return NULL;
    }
    Py_INCREF(value);
    return value;
    
}

static PyMappingMethods SmartDict_as_mapping ={
    .mp_length=0,
    .mp_subscript=SmartDict_getitem,
    .mp_ass_subscript=SmartDict_setitem
};

static PyTypeObject SmartDictType ={
    PyVarObject_HEAD_INIT(NULL,0)
    .tp_name="smart_dict.SmartDict",
    .tp_basicsize=sizeof(SmartDictObject),
    .tp_itemsize =0,
    .tp_dealloc=(destructor)SmartDict_dealloc,
    .tp_flags=Py_TPFLAGS_DEFAULT,
    .tp_doc="SmartDict object",
    .tp_as_mapping=&SmartDict_as_mapping,
    .tp_new=SmartDict_new,
};

static PyModuleDef smartdictmodule=
{
    PyModuleDef_HEAD_INIT,
    "smart_dict",
    "Smart Dictionary Module",
    -1,
};

PyMODINIT_FUNC PyInit_smart_dict(void)
{
    PyObject *m;
    if(PyType_Ready(&SmartDictType)<0)
        return NULL;
    
    m=PyModule_Create(&smartdictmodule);
    if(!m)
        return NULL;
    
    Py_INCREF(&SmartDictType);
    PyModule_AddObject(m,"SmartDict",(PyObject *)&SmartDictType);
    return m;
}