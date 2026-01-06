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
    PyObject *list;
    if(value== NULL)
    {
        PyErr_SetString(PyExc_TypeError,"Cannot delete items");
        return -1;
    }
    list=PyDict_GetItem(sd->dict,key);

    if(list==NULL)
    {
        list=PyList_New(0);
        if(!list) return -1;
        PyDict_SetItem(sd->dict,key,list);
        Py_DECREF(list);
    }

    return PyList_Append(list,value);
}

static PyObject* SmartDict_getitem(PyObject *self,PyObject *key)
{
    SmartDictObject *sd=(SmartDictObject *)self;
    PyObject *list;
    Py_ssize_t size;

    list=PyDict_GetItem(sd->dict,key);
    if(!list){
        PyErr_SetString(PyExc_KeyError,"Key Not Found");
        return NULL;
    }

    size=PyList_Size(list);
    if(size==0)
    {
        PyErr_SetString(PyExc_KeyError,"No Values Stored");
        return NULL;
    }

    PyObject *value=PyList_GetItem(list,size-1);
    Py_INCREF(value);
    return value;
    
}

static PyObject* SmartDict_get(PyObject *self,PyObject *args)
{
    const char *key;
    int version;
    SmartDictObject *sd=(SmartDictObject *)self;
    PyObject *list;

    if(!PyArg_ParseTuple(args,"si",&key,&version))
        return NULL;
    
    list =PyDict_GetItemString(sd->dict,key);
    if(!list)
    {
        PyErr_SetString(PyExc_KeyError,"Key Not Found");
        return NULL;
    }
    if(version<=0 ||version>PyList_Size(list))
    {
        PyErr_SetString(PyExc_IndexError,"Invalid version");
        return NULL;
    }

    PyObject *value=PyList_GetItem(list,version-1);
    Py_INCREF(value);
    return value;
}

static PyObject* SmartDict_delete(PyObject *self,PyObject *args)
{
    const char *key;
    int version =-1;
    SmartDictObject *sd=(SmartDictObject *)self;
    PyObject *list;

    PyArg_ParseTuple(args,"s|i",&key,&version);

    list =PyDict_GetItemString(sd->dict,key);
    if(!list)
    {
        PyErr_SetString(PyExc_KeyError,"Key Not Found");
        return NULL;
    }
    if(version==-1)
    {
        PyDict_DelItemString(sd->dict,key);
        Py_RETURN_NONE;
    }
    if(version<=0||version>PyList_Size(list))
    {
        PyErr_SetString(PyExc_IndexError,"Invalid version");
        return NULL;
    }

    PySequence_DelItem(list,version-1);
    Py_RETURN_NONE;

}

static PyMappingMethods SmartDict_as_mapping ={
    .mp_length=0,
    .mp_subscript=SmartDict_getitem,
    .mp_ass_subscript=SmartDict_setitem
};

static PyMethodDef SmartDict_methods[]=
{
    {"get",SmartDict_get,METH_VARARGS,"Get Specific Version"},
    {"delete",SmartDict_delete,METH_VARARGS,"Delete Key or Version"},
    {NULL}
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
    .tp_methods=SmartDict_methods,
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