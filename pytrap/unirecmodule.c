#include <Python.h>
#include <structmember.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "fields.h"

UR_FIELDS()

extern PyObject *TrapError;

/*********************/
/*    UnirecTime     */
/*********************/
static PyTypeObject pytrap_UnirecTime;

typedef struct {
    PyObject_HEAD
    ur_time_t timestamp;
} pytrap_unirectime;

static PyObject *
UnirecTime_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    //int PyObject_IsInstance(PyObject *inst, PyObject *cls)
    if (a->ob_type != &pytrap_UnirecTime || b->ob_type != &pytrap_UnirecTime) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirectime *ur_a = (pytrap_unirectime *) a;
    pytrap_unirectime *ur_b = (pytrap_unirectime *) b;

    switch (op) {
    case Py_EQ:
        result = (ur_a->timestamp == ur_b->timestamp ? Py_True : Py_False);
    case Py_NE:
        result = (ur_a->timestamp != ur_b->timestamp ? Py_True : Py_False);
    case Py_LE:
        result = (ur_a->timestamp <= ur_b->timestamp ? Py_True : Py_False);
    case Py_GE:
        result = (ur_a->timestamp >= ur_b->timestamp ? Py_True : Py_False);
    case Py_LT:
        result = (ur_a->timestamp < ur_b->timestamp ? Py_True : Py_False);
    case Py_GT:
        result = (ur_a->timestamp > ur_b->timestamp ? Py_True : Py_False);
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyMethodDef pytrap_unirectime_methods[] = {
    {NULL, NULL, 0, NULL}
};

static PyObject *
UnirecTime_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirectime *self;
    uint32_t secs, msecs;

    self = (pytrap_unirectime *) type->tp_alloc(type, 0);
    if (self != NULL) {
        if (!PyArg_ParseTuple(args, "II", &secs, &msecs)) {
            Py_DECREF(self);
            return NULL;
        }
        self->timestamp = ur_time_from_sec_msec(secs, msecs);
    }

    return (PyObject *) self;
}

static PyObject *
UnirecTime_str(pytrap_unirectime *self)
{
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("%u.%03u", ur_time_get_sec(self->timestamp),
        ur_time_get_msec(self->timestamp));
#else
    return PyString_FromFormat("%u.%03u", ur_time_get_sec(self->timestamp),
        ur_time_get_msec(self->timestamp));
#endif
}

long
UnirecTime_hash(pytrap_unirectime *o)
{
    return (long) o->timestamp;
}

static PyTypeObject pytrap_UnirecTime = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecTime",          /* tp_name */
    sizeof(pytrap_unirectime),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecTime_hash,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecTime_str,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Class for UniRec timestamp storage and base data access.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc) UnirecTime_compare,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unirectime_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    UnirecTime_new,                         /* tp_new */
};


/*********************/
/*    UnirecIPAddr   */
/*********************/
static PyTypeObject pytrap_UnirecIPAddr;

typedef struct {
    PyObject_HEAD
    ip_addr_t ip;
} pytrap_unirecipaddr;

static PyObject *
UnirecIPAddr_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    //int PyObject_IsInstance(PyObject *inst, PyObject *cls)
    if (a->ob_type != &pytrap_UnirecIPAddr || b->ob_type != &pytrap_UnirecIPAddr) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirecipaddr *ur_a = (pytrap_unirecipaddr *) a;
    pytrap_unirecipaddr *ur_b = (pytrap_unirecipaddr *) b;

    char str1[INET6_ADDRSTRLEN];
    char str2[INET6_ADDRSTRLEN];
    ip_to_str(&ur_a->ip, str1);
    ip_to_str(&ur_b->ip, str2);

    int res = ip_cmp(&ur_a->ip, &ur_b->ip);

    switch (op) {
    case Py_EQ:
        result = (res == 0 ? Py_True : Py_False);
    case Py_NE:
        result = (res != 0 ? Py_True : Py_False);
    case Py_LE:
        result = (res <= 0 ? Py_True : Py_False);
    case Py_GE:
        result = (res >= 0 ? Py_True : Py_False);
    case Py_LT:
        result = (res < 0 ? Py_True : Py_False);
    case Py_GT:
        result = (res > 0 ? Py_True : Py_False);
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyMethodDef pytrap_unirecipaddr_methods[] = {
    {NULL, NULL, 0, NULL}
};

int
UnirecIPAddr_init(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
    pytrap_unirecipaddr *s = (pytrap_unirecipaddr *) self;
    char *ip_str;

    if (s != NULL) {
        if (!PyArg_ParseTuple(args, "s", &ip_str)) {
            return EXIT_FAILURE;
        }
        if (ip_from_str(ip_str, &s->ip) != 1) {
            PyErr_SetString(TrapError, "Could not parse given IP address.");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;

}

static PyObject *
UnirecIPAddr_str(pytrap_unirecipaddr *self)
{
    char str[INET6_ADDRSTRLEN];
    ip_to_str(&self->ip, str);
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(str);
#else
    return PyString_FromString(str);
#endif
}

long
UnirecIPAddr_hash(pytrap_unirecipaddr *o)
{
    if (ip_is4(&o->ip)) {
        return (long) o->ip.ui32[2];
    } else {
        return (long) (o->ip.ui64[0] ^ o->ip.ui64[1]);
    }
}

static PyTypeObject pytrap_UnirecIPAddr = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecIPAddr",          /* tp_name */
    sizeof(pytrap_unirecipaddr),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecIPAddr_hash,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecIPAddr_str,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Class for UniRec IP Address storage and base data access.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc) UnirecIPAddr_compare,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unirecipaddr_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) UnirecIPAddr_init,                         /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};


/*********************/
/* UnirecTemplate    */
/*********************/


typedef struct {
    PyObject_HEAD
    ur_template_t *urtmplt;
} pytrap_unirectemplate;

static PyObject *
UnirecTemplate_get(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    uint32_t field_id;
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"field_id", "data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IO!", kwlist, &field_id, &PyBytes_Type, &dataObj)) {
        return NULL;
    }

    PyBytes_AsStringAndSize(dataObj, &data, &data_size);

    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);

    switch (ur_get_type(field_id)) {
    case UR_TYPE_UINT8:
        return Py_BuildValue("B", *(uint8_t *)value);
        break;
    case UR_TYPE_UINT16:
        return Py_BuildValue("H", *(uint16_t *)value);
        break;
    case UR_TYPE_UINT32:
        return Py_BuildValue("I", *(uint32_t *)value);
        break;
    case UR_TYPE_UINT64:
        return Py_BuildValue("K", *(uint64_t *)value);
        break;
    case UR_TYPE_INT8:
        return Py_BuildValue("c", *(int8_t *)value);
        break;
    case UR_TYPE_INT16:
        return Py_BuildValue("h", *(int16_t *)value);
        break;
    case UR_TYPE_INT32:
        return Py_BuildValue("i", *(int32_t *)value);
        break;
    case UR_TYPE_INT64:
        return Py_BuildValue("L", *(int64_t *)value);
        break;
    case UR_TYPE_CHAR:
        return Py_BuildValue("b", *(char *)value);
        break;
    case UR_TYPE_FLOAT:
        return Py_BuildValue("f", *(float *)value);
        break;
    case UR_TYPE_DOUBLE:
        return Py_BuildValue("d", *(double *)value);
        break;
    case UR_TYPE_IP:
        {
            pytrap_unirecipaddr *new_ip = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);
            memcpy(&new_ip->ip, value, sizeof(ip_addr_t));
            //ip_addr_t *ip_p = (ip_addr_t *) value;
            //new_ip->ip.ui32[0] = htonl(ip_p->ui32[3]);
            //new_ip->ip.ui32[1] = htonl(ip_p->ui32[2]);
            //new_ip->ip.ui32[2] = htonl(ip_p->ui32[1]);
            //new_ip->ip.ui32[3] = htonl(ip_p->ui32[0]);
            return (PyObject *) new_ip;
        }
    case UR_TYPE_TIME:
        {
            pytrap_unirectime *new_time = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
            new_time->timestamp = *((ur_time_t *) value);
            return (PyObject *) new_time;
        }
    case UR_TYPE_STRING:
        {
            Py_ssize_t value_size = ur_get_var_len(self->urtmplt, data, field_id);
            //fprintf(stderr, "varlensize: %lu\n", value_size);
#if PY_MAJOR_VERSION >= 3
            return PyUnicode_FromStringAndSize(value, value_size);
#else
            return PyString_FromStringAndSize(value, value_size);
#endif
        }
        break;
    case UR_TYPE_BYTES:
        /* TODO not implemented */
        break;
    default:
        {
            // Unknown type - print the value in hex
            //int size = ur_get_len(templates[index], rec, id);
            //fprintf(file, "0x");
            //for (int i = 0; i < size; i++) {
            //    fprintf(file, "%02x", ((unsigned char*)value)[i]);
            //}
        }
        break;
    } // case (field type)
    Py_RETURN_NONE;
}

static PyMethodDef pytrap_unirectemplate_methods[] = {
        {"get", (PyCFunction) UnirecTemplate_get, METH_VARARGS | METH_KEYWORDS, ""},
        {NULL, NULL, 0, NULL}
};

static PyObject *
UnirecTemplate_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirectemplate *self;
    const char *spec;
    // TODO fix this
    //char *errstring;

    //fprintf(stderr, "alloc()\n");
    self = (pytrap_unirectemplate *) type->tp_alloc(type, 0);
    if (self != NULL) {
        static char *kwlist[] = {"spec", NULL};
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &spec)) {
            Py_DECREF(self);
            return NULL;
        }
        self->urtmplt = NULL;
        //fprintf(stderr, "spec received: %s\n", spec);
        int ret;
        if ((ret = ur_define_set_of_fields(spec)) != UR_OK) {
            /* TODO handle error */
            fprintf(stderr, "ur_define_set_of_fields error\n");
        }
        //fprintf(stderr, "ur_define_set_of_fields %i\n", ret);
        /* TODO fix this */
        //self->urtmplt = ur_create_template(spec, &errstring);
        self->urtmplt = ur_create_template_from_ifc_spec(spec);
        if (self->urtmplt == NULL) {
            //PyErr_SetString(TrapError, errstring);
            PyErr_SetString(TrapError, "Creation of UniRec template failed.");
            //free(errstring);
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *) self;
}

static void UnirecTemplate_dealloc(pytrap_unirectemplate *self)
{
    //fprintf(stderr, "dealloc()\n");
    ur_free_template(self->urtmplt);
    Py_DECREF(self);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyTypeObject pytrap_UnirecTemplate = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecTemplate",          /* tp_name */
    sizeof(pytrap_unirectemplate),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) UnirecTemplate_dealloc,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Class for UniRec template storage and base data access.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unirectemplate_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    UnirecTemplate_new,                         /* tp_new */
};

/**
 * \brief Initialize UniRec template class and add it to pytrap module.
 *
 * \param [in,out] m    pointer to the module Object
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int init_unirectemplate(PyObject *m)
{
    /* Add Time */
    if (PyType_Ready(&pytrap_UnirecTime) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecTime);
    PyModule_AddObject(m, "UnirecTime", (PyObject *) &pytrap_UnirecTime);

    /* Add IPAddr */
    //pytrap_UnirecTemplate.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pytrap_UnirecIPAddr) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecIPAddr);
    PyModule_AddObject(m, "UnirecIPAddr", (PyObject *) &pytrap_UnirecIPAddr);


    /* Add Template */
    if (PyType_Ready(&pytrap_UnirecTemplate) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecTemplate);
    PyModule_AddObject(m, "UnirecTemplate", (PyObject *) &pytrap_UnirecTemplate);


    return EXIT_SUCCESS;
}


