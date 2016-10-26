#include <Python.h>
#include <datetime.h>
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

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecTime) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecTime)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirectime *ur_a = (pytrap_unirectime *) a;
    pytrap_unirectime *ur_b = (pytrap_unirectime *) b;

    switch (op) {
    case Py_EQ:
        result = (ur_a->timestamp == ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_NE:
        result = (ur_a->timestamp != ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_LE:
        result = (ur_a->timestamp <= ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_GE:
        result = (ur_a->timestamp >= ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_LT:
        result = (ur_a->timestamp < ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_GT:
        result = (ur_a->timestamp > ur_b->timestamp ? Py_True : Py_False);
        break;
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyObject *
UnirecTime_getSeconds(pytrap_unirectime *self)
{
    return PyLong_FromLong(ur_time_get_sec(self->timestamp));
}

static PyObject *
UnirecTime_getMiliSeconds(pytrap_unirectime *self)
{
    return PyLong_FromLong(ur_time_get_msec(self->timestamp));
}

static PyObject *
UnirecTime_getTimeAsFloat(pytrap_unirectime *self)
{
    double t = (double) ur_time_get_sec(self->timestamp);
    t += (double) ur_time_get_msec(self->timestamp) / 1000;
    return PyFloat_FromDouble(t);
}
static PyObject *
UnirecTime_now(pytrap_unirectime *self)
{
    pytrap_unirectime *result;
    struct timeval t;

    result = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
    if (result != NULL) {
        if (gettimeofday(&t, NULL) == 0) {
            result->timestamp = ur_time_from_sec_msec(t.tv_sec, t.tv_usec / 1000);
        } else {
            PyErr_SetString(TrapError, "Could not get current time.");
        }
    } else {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate UnirecTime.");
    }

    return (PyObject *) result;
}

static PyObject *
UnirecTime_fromDatetime(pytrap_unirectime *self, PyObject *args)
{
    pytrap_unirectime *result;
    PyObject *dt;

    if (!PyArg_ParseTuple(args, "O", &dt)) {
        return NULL;
    }
    if (!PyDateTime_CheckExact(dt)) {
        return NULL;
    }

    PyObject *strftime = PyUnicode_FromString("strftime");
    PyObject *utformat = PyUnicode_FromString("%s");
    PyObject *secsObj =  PyObject_CallMethodObjArgs(dt, strftime, utformat, NULL);
#if PY_MAJOR_VERSION >= 3
    PyObject *secsLong = PyLong_FromUnicodeObject(secsObj, 10);
#else
    PyObject *secsLong = PyNumber_Long(secsObj);
#endif
    uint32_t secs = (uint32_t) PyLong_AsLong(secsLong);
    uint32_t microsec = PyDateTime_DATE_GET_MICROSECOND(dt);
    Py_DECREF(strftime);
    Py_DECREF(utformat);
    Py_DECREF(secsObj);
    Py_DECREF(secsLong);

    result = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
    if (result != NULL) {
        result->timestamp = ur_time_from_sec_msec(secs, microsec / 1000);
    } else {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate UnirecTime.");
    }

    return (PyObject *) result;
}

static PyObject *
UnirecTime_toDatetime(pytrap_unirectime *self)
{
    PyObject *result;
    const struct tm *t;
    time_t ts = ur_time_get_sec(self->timestamp);
    t = gmtime(&ts);
    result = PyDateTime_FromDateAndTime(1900 + t->tm_year, t->tm_mon + 1, t->tm_mday,
                                        t->tm_hour, t->tm_min, t->tm_sec,
                                        ur_time_get_msec(self->timestamp) * 1000);
    return result;
}

static PyObject *
UnirecTime_format(pytrap_unirectime *self, PyObject *args)
{
    PyObject *fmt = NULL;
    if (!PyArg_ParseTuple(args, "|O", &fmt)) {
        return NULL;
    }
    PyObject *dt = UnirecTime_toDatetime(self);

    if (fmt != NULL) {
#if PY_MAJOR_VERSION >= 3
        if (!PyUnicode_Check(fmt))
#else
        if (!PyUnicode_Check(fmt) && !PyString_Check(fmt))
#endif
        {
            PyErr_SetString(PyExc_TypeError, "Argument field_name must be string.");
            return NULL;
        }
    } else {
        fmt = PyUnicode_FromString("%FT%TZ");
    }
    PyObject *strftime = PyUnicode_FromString("strftime");

    PyObject *result = PyObject_CallMethodObjArgs(dt, strftime, fmt, NULL);
    Py_DECREF(strftime);
    return result;
}

static PyMethodDef pytrap_unirectime_methods[] = {
    {"fromDatetime", (PyCFunction) UnirecTime_fromDatetime, METH_STATIC | METH_VARARGS,
        "Get UnirecTime from a datetime object.\n\n"
        "Returns:\n"
        "    (UnirecTime): Retrieved timestamp.\n"
    },
    {"getSeconds", (PyCFunction) UnirecTime_getSeconds, METH_NOARGS,
        "Get number of seconds of timestamp.\n\n"
        "Returns:\n"
        "    (long): Retrieved number of seconds.\n"
    },
    {"getMiliSeconds", (PyCFunction) UnirecTime_getMiliSeconds, METH_NOARGS,
        "Get number of seconds of timestamp.\n\n"
        "Returns:\n"
        "    (long): Retrieved number of seconds.\n"
    },
    {"getTimeAsFloat", (PyCFunction) UnirecTime_getTimeAsFloat, METH_NOARGS,
        "Get number of seconds of timestamp.\n\n"
        "Returns:\n"
        "    (double): Retrieved timestamp as floating point number.\n"
    },
    {"toDatetime", (PyCFunction) UnirecTime_toDatetime, METH_NOARGS,
        "Get timestamp as a datetime object.\n\n"
        "Returns:\n"
        "    (datetime): Retrieved timestamp as datetime.\n"
    },
    {"format", (PyCFunction) UnirecTime_format, METH_VARARGS,
        "Get timestamp as a datetime object.\n\n"
        "Args:\n"
        "    format (Optional[str]): Formatting string, same as for datetime.strftime (default: \"%FT%TZ\").\n\n"
        "Returns:\n"
        "    (str): Formatted timestamp as string.\n"
    },
    {"now", (PyCFunction) UnirecTime_now, METH_STATIC | METH_NOARGS,
        "Get UnirecTime instance of current time.\n\n"
        "Returns:\n"
        "    (UnirecTime): Current date and time.\n"
    },

    {NULL, NULL, 0, NULL}
};

int
UnirecTime_init(pytrap_unirectime *s, PyObject *args, PyObject *kwds)
{
    uint32_t secs, msecs = 0;

    if (s != NULL) {
        if (!PyArg_ParseTuple(args, "I|I", &secs, &msecs)) {
            return -1;
        }
        s->timestamp = ur_time_from_sec_msec(secs, msecs);
    } else {
        return -1;
    }

    return 0;
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

static PyObject *
UnirecTime_repr(pytrap_unirectime *self)
{
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("UnirecTime(%u, %u)", ur_time_get_sec(self->timestamp),
        ur_time_get_msec(self->timestamp));
#else
    return PyString_FromFormat("UnirecTime(%u, %u)", ur_time_get_sec(self->timestamp),
        ur_time_get_msec(self->timestamp));
#endif
}

long
UnirecTime_hash(pytrap_unirectime *o)
{
    return (long) o->timestamp;
}

static PyObject *
UnirecTime_nb_add(PyObject *a, PyObject *b)
{
    uint64_t sec, msec;
    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecTime)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be of UnirecTime type.");
        return NULL;
    }

    pytrap_unirectime *ur_a = (pytrap_unirectime *) a;
    msec = ur_time_get_msec(ur_a->timestamp);
    sec = ur_time_get_sec(ur_a->timestamp);
    if (PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecTime)) {
        pytrap_unirectime *ur_b = (pytrap_unirectime *) b;
        msec += ur_time_get_msec(ur_b->timestamp);
        sec += ur_time_get_sec(ur_b->timestamp) + (msec / 1000);
        msec %= 1000;
    } else if (PyLong_Check(b)) {
        int64_t tmp_b = PyLong_AsLong(b);
        sec += tmp_b;
#if PY_MAJOR_VERSION < 3
    } else if (PyInt_Check(b)) {
        int64_t tmp_b = PyInt_AsLong(b);
        sec += tmp_b;
#endif
    } else {
        PyErr_SetString(PyExc_TypeError, "Unsupported type for this operation.");
        return NULL;
    }

    ur_a = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
    ur_a->timestamp = ur_time_from_sec_msec((uint32_t) sec, (uint32_t) msec);
    return (PyObject *) ur_a;
}

static PyObject *
UnirecTime_nb_float(pytrap_unirectime *self)
{
    double res = (double) ur_time_get_sec(self->timestamp);
    res += (double) ur_time_get_msec(self->timestamp) / 1000;
    return PyFloat_FromDouble(res);
}

#if PY_MAJOR_VERSION >= 3
static PyNumberMethods UnirecTime_numbermethods = {
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_add; */
     0, /* binaryfunc nb_subtract; */
     0, /* binaryfunc nb_multiply; */
     0, /* binaryfunc nb_remainder; */
     0, /* binaryfunc nb_divmod; */
     0, /* ternaryfunc nb_power; */
     0, /* unaryfunc nb_negative; */
     0, /* unaryfunc nb_positive; */
     0, /* unaryfunc nb_absolute; */
     0, /* inquiry nb_bool; */
     0, /* unaryfunc nb_invert; */
     0, /* binaryfunc nb_lshift; */
     0, /* binaryfunc nb_rshift; */
     0, /* binaryfunc nb_and; */
     0, /* binaryfunc nb_xor; */
     0, /* binaryfunc nb_or; */
     0, /* unaryfunc nb_int; */
     0, /* void *nb_reserved; */
     (unaryfunc) UnirecTime_nb_float, /* unaryfunc nb_float; */
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_inplace_add; */
     0, /* binaryfunc nb_inplace_subtract; */
     0, /* binaryfunc nb_inplace_multiply; */
     0, /* binaryfunc nb_inplace_remainder; */
     0, /* ternaryfunc nb_inplace_power; */
     0, /* binaryfunc nb_inplace_lshift; */
     0, /* binaryfunc nb_inplace_rshift; */
     0, /* binaryfunc nb_inplace_and; */
     0, /* binaryfunc nb_inplace_xor; */
     0, /* binaryfunc nb_inplace_or; */
     0, /* binaryfunc nb_floor_divide; */
     0, /* binaryfunc nb_true_divide; */
     0, /* binaryfunc nb_inplace_floor_divide; */
     0, /* binaryfunc nb_inplace_true_divide; */
     0  /* unaryfunc nb_index; */
};
#else
static PyNumberMethods UnirecTime_numbermethods = {
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_add; */
     0, /* binaryfunc nb_subtract; */
     0, /* binaryfunc nb_multiply; */
     0, /* binaryfunc nb_divide; */
     0, /* binaryfunc nb_remainder; */
     0, /* binaryfunc nb_divmod; */
     0, /* ternaryfunc nb_power; */
     0, /* unaryfunc nb_negative; */
     0, /* unaryfunc nb_positive; */
     0, /* unaryfunc nb_absolute; */
     0, /* inquiry nb_nonzero; */
     0, /* unaryfunc nb_invert; */
     0, /* binaryfunc nb_lshift; */
     0, /* binaryfunc nb_rshift; */
     0, /* binaryfunc nb_and; */
     0, /* binaryfunc nb_xor; */
     0, /* binaryfunc nb_or; */
     0, /* coercion nb_coerce; */
     0, /* unaryfunc nb_int; */
     0, /* unaryfunc nb_long; */
     (unaryfunc) UnirecTime_nb_float, /* unaryfunc nb_float; */
     0, /* unaryfunc nb_oct; */
     0, /* unaryfunc nb_hex; */
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_inplace_add; */
     0, /* binaryfunc nb_inplace_subtract; */
     0, /* binaryfunc nb_inplace_multiply; */
     0, /* binaryfunc nb_inplace_divide; */
     0, /* binaryfunc nb_inplace_remainder; */
     0, /* ternaryfunc nb_inplace_power; */
     0, /* binaryfunc nb_inplace_lshift; */
     0, /* binaryfunc nb_inplace_rshift; */
     0, /* binaryfunc nb_inplace_and; */
     0, /* binaryfunc nb_inplace_xor; */
     0, /* binaryfunc nb_inplace_or; */
     0, /* binaryfunc nb_floor_divide; */
     0, /* binaryfunc nb_true_divide; */
     0, /* binaryfunc nb_inplace_floor_divide; */
     0, /* binaryfunc nb_inplace_true_divide; */
     0, /* unaryfunc nb_index; */
};
#endif

static PyTypeObject pytrap_UnirecTime = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecTime", /* tp_name */
    sizeof(pytrap_unirectime), /* tp_basicsize */
    0, /* tp_itemsize */
    0, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_reserved */
    (reprfunc) UnirecTime_repr, /* tp_repr */
    &UnirecTime_numbermethods, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    (hashfunc) UnirecTime_hash, /* tp_hash  */
    0, /* tp_call */
    (reprfunc) UnirecTime_str, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
#if PY_MAJOR_VERSION < 3
        Py_TPFLAGS_CHECKTYPES |
#endif
        Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Class for UniRec timestamp storage and base data access.", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    (richcmpfunc) UnirecTime_compare, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    pytrap_unirectime_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    (initproc) UnirecTime_init, /* tp_init */
    0, /* tp_alloc */
    PyType_GenericNew, /* tp_new */
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

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecIPAddr) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecIPAddr)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirecipaddr *ur_a = (pytrap_unirecipaddr *) a;
    pytrap_unirecipaddr *ur_b = (pytrap_unirecipaddr *) b;

    int res = ip_cmp(&ur_a->ip, &ur_b->ip);

    switch (op) {
    case Py_EQ:
        result = (res == 0 ? Py_True : Py_False);
        break;
    case Py_NE:
        result = (res != 0 ? Py_True : Py_False);
        break;
    case Py_LE:
        result = (res <= 0 ? Py_True : Py_False);
        break;
    case Py_GE:
        result = (res >= 0 ? Py_True : Py_False);
        break;
    case Py_LT:
        result = (res < 0 ? Py_True : Py_False);
        break;
    case Py_GT:
        result = (res > 0 ? Py_True : Py_False);
        break;
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyObject *
UnirecIPAddr_isIPv4(pytrap_unirecipaddr *self)
{
    if (ip_is4(&self->ip)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
UnirecIPAddr_isIPv6(pytrap_unirecipaddr *self)
{
    if (ip_is4(&self->ip)) {
        Py_RETURN_FALSE;
    } else {
        Py_RETURN_TRUE;
    }
}

static PyObject *
UnirecIPAddr_isNull(pytrap_unirecipaddr *self)
{
    if (ip_is_null(&self->ip)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static int
UnirecIPAddr_bool(pytrap_unirecipaddr *self)
{
    /* bool(ip) == (not isNull(ip)) */
    if (ip_is_null(&self->ip)) {
        return 0;
    } else {
        return 1;
    }
}

static PyObject *
UnirecIPAddr_inc(pytrap_unirecipaddr *self)
{
    pytrap_unirecipaddr * ip_inc;
    ip_inc = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);

    if (ip_is6(&self->ip)) {
        memcpy(&ip_inc->ip, &self->ip, 16);

        uint32_t tmp = 0xffffffff;
        int i;
        for (i = 3; i >= 0; i--) {
            ip_inc->ip.ui32[i] = htonl(ntohl(self->ip.ui32[i]) + 1);
            if (self->ip.ui32[i] < tmp) {
                break;
            }
        }
    } else {
        ip_inc->ip.ui64[0] = 0;
        ip_inc->ip.ui32[2] = htonl(ntohl(self->ip.ui32[2]) + 1);
        ip_inc->ip.ui32[3] = 0xffffffff;
    }
    Py_INCREF(ip_inc);
    return (PyObject *) ip_inc;
}

static PyObject *
UnirecIPAddr_dec(pytrap_unirecipaddr *self)
{
    pytrap_unirecipaddr * ip_dec;
    ip_dec = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);

    if (ip_is6(&self->ip)) {
        memcpy(&ip_dec->ip, &self->ip, 16);

        uint32_t tmp = 0xffffffff;
        int i;
        for (i = 3; i >=0; i--) {
            ip_dec->ip.ui32[i] = htonl(ntohl(self->ip.ui32[i]) - 1);
            if (ip_dec->ip.ui32[i] != tmp) {
                break;
            }
        }
    } else {
        ip_dec->ip.ui64[0] = 0;
        ip_dec->ip.ui32[2] = htonl(ntohl(self->ip.ui32[2]) - 1);
        ip_dec->ip.ui32[3] = 0xffffffff;
    }
    Py_INCREF(ip_dec);
    return (PyObject *) ip_dec;
}

static PyMethodDef pytrap_unirecipaddr_methods[] = {
    {"isIPv4", (PyCFunction) UnirecIPAddr_isIPv4, METH_NOARGS,
        "Check if the address is IPv4.\n\n"
        "Returns:\n"
        "    bool: True if the address is IPv4.\n"
        },

    {"isIPv6", (PyCFunction) UnirecIPAddr_isIPv6, METH_NOARGS,
        "Check if the address is IPv6.\n\n"
        "Returns:\n"
        "    bool: True if the address is IPv6.\n"
        },

    {"isNull", (PyCFunction) UnirecIPAddr_isNull, METH_NOARGS,
        "Check if the address is null (IPv4 or IPv6), i.e. \"0.0.0.0\" or \"::\".\n\n"
        "Returns:\n"
        "    bool: True if the address is null.\n"
        },

    {"inc", (PyCFunction) UnirecIPAddr_inc, METH_NOARGS,
        "Increment IP address.\n\n"
        "Returns:\n"
        "    UnirecIPAddr: New incremented IPAddress.\n"
        },

    {"dec", (PyCFunction) UnirecIPAddr_dec, METH_NOARGS,
        "Decrement IP address.\n\n"
        "Returns:\n"
        "    UnirecIPAddr: New decremented IPAddress.\n"
        },
    {NULL, NULL, 0, NULL}
};

static PyNumberMethods UnirecIPAddr_numbermethods = {
#if PY_MAJOR_VERSION >= 3
    .nb_bool = (inquiry) UnirecIPAddr_bool, 
#else
    .nb_nonzero = (inquiry) UnirecIPAddr_bool,
#endif
};

int
UnirecIPAddr_init(pytrap_unirecipaddr *s, PyObject *args, PyObject *kwds)
{
    char *ip_str;

    if (s != NULL) {
        if (!PyArg_ParseTuple(args, "s", &ip_str)) {
            return -1;
        }
        if (ip_from_str(ip_str, &s->ip) != 1) {
            PyErr_SetString(TrapError, "Could not parse given IP address.");
            return -1;
        }
    } else {
        return -1;
    }
    return 0;

}

static PyObject *
UnirecIPAddr_repr(pytrap_unirecipaddr *self)
{
    char str[INET6_ADDRSTRLEN];
    ip_to_str(&self->ip, str);
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("UnirecIPAddr('%s')", str);
#else
    return PyString_FromFormat("UnirecIPAddr('%s')", str);
#endif
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
    (reprfunc) UnirecIPAddr_repr, /* tp_repr */
    &UnirecIPAddr_numbermethods,                         /* tp_as_number */
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
    char *data;
    Py_ssize_t data_size;
    PyObject *data_obj; // Pointer to object containing the data we are pointing to
    PyDictObject *urdict;

    /* for iteration */
    Py_ssize_t iter_index;
    Py_ssize_t field_count;
} pytrap_unirectemplate;

static inline PyObject *
UnirecTemplate_get_local(pytrap_unirectemplate *self, char *data, int32_t field_id)
{
    if (data == NULL) {
        PyErr_SetString(TrapError, "Data was not set yet.");
        return NULL;
    }
    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);

    switch (ur_get_type(field_id)) {
    case UR_TYPE_UINT8:
        return Py_BuildValue("B", *(uint8_t *) value);
        break;
    case UR_TYPE_UINT16:
        return Py_BuildValue("H", *(uint16_t *) value);
        break;
    case UR_TYPE_UINT32:
        return Py_BuildValue("I", *(uint32_t *) value);
        break;
    case UR_TYPE_UINT64:
        return Py_BuildValue("K", *(uint64_t *) value);
        break;
    case UR_TYPE_INT8:
        return Py_BuildValue("c", *(int8_t *) value);
        break;
    case UR_TYPE_INT16:
        return Py_BuildValue("h", *(int16_t *) value);
        break;
    case UR_TYPE_INT32:
        return Py_BuildValue("i", *(int32_t *) value);
        break;
    case UR_TYPE_INT64:
        return Py_BuildValue("L", *(int64_t *) value);
        break;
    case UR_TYPE_CHAR:
        return Py_BuildValue("b", *(char *) value);
        break;
    case UR_TYPE_FLOAT:
        return Py_BuildValue("f", *(float *) value);
        break;
    case UR_TYPE_DOUBLE:
        return Py_BuildValue("d", *(double *) value);
        break;
    case UR_TYPE_IP:
        {
            pytrap_unirecipaddr *new_ip = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);
            memcpy(&new_ip->ip, value, sizeof(ip_addr_t));
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
#if PY_MAJOR_VERSION >= 3
            return PyUnicode_FromStringAndSize(value, value_size);
#else
            return PyString_FromStringAndSize(value, value_size);
#endif
        }
        break;
    case UR_TYPE_BYTES:
        {
            Py_ssize_t value_size = ur_get_var_len(self->urtmplt, data, field_id);
            return PyByteArray_FromStringAndSize(value, value_size);
        }
        break;
    default:
        PyErr_SetString(PyExc_NotImplementedError, "Unknown UniRec field type.");
        return NULL;
    } // case (field type)
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_getByID(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    int32_t field_id;
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", "field_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "OI", kwlist, &dataObj, &field_id)) {
        return NULL;
    }

    if (PyByteArray_Check(dataObj)) {
        //data_size = PyByteArray_Size(dataObj);
        data = PyByteArray_AsString(dataObj);
    } else if (PyBytes_Check(dataObj)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

    return UnirecTemplate_get_local(self, data, field_id);
}

static inline int32_t
UnirecTemplate_get_field_id(pytrap_unirectemplate *self, PyObject *name)
{
    PyObject *v = PyDict_GetItem((PyObject *) self->urdict, name);

    if (v == NULL) {
        return UR_ITER_END;
    }

    int32_t field_id = (int32_t) PyLong_AsLong(v);

    return field_id;
}

static PyObject *
UnirecTemplate_getByName(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    int32_t field_id;
    PyObject *dataObj, *field_name;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", "field_name", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "OO", kwlist, &dataObj, &field_name)) {
        return NULL;
    }

    if (PyByteArray_Check(dataObj)) {
        //data_size = PyByteArray_Size(dataObj);
        data = PyByteArray_AsString(dataObj);
    } else if (PyBytes_Check(dataObj)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    if (!PyUnicode_Check(field_name))
#else
    if (!PyUnicode_Check(field_name) && !PyString_Check(field_name))
#endif
    {
        PyErr_SetString(PyExc_TypeError, "Argument field_name must be string.");
        return NULL;
    }

    field_id = UnirecTemplate_get_field_id(self, field_name);
    if (field_id == UR_ITER_END) {
        PyErr_SetString(TrapError, "Field was not found.");
        return NULL;
    }
    return UnirecTemplate_get_local(self, data, field_id);
}

static inline PyObject *
UnirecTemplate_set_local(pytrap_unirectemplate *self, char *data, int32_t field_id, PyObject *valueObj)
{
    if (data == NULL) {
        PyErr_SetString(TrapError, "Data was not set yet.");
        return NULL;
    }
    PY_LONG_LONG longval;
    double floatval;
    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);

    switch (ur_get_type(field_id)) {
    case UR_TYPE_UINT8:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint8_t *) value) = (uint8_t) longval;
        }
        break;
    case UR_TYPE_UINT16:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint16_t *) value) = (uint16_t) longval;
        }
        break;
    case UR_TYPE_UINT32:
        longval = PyLong_AsLongLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint32_t *) value) = (uint32_t) longval;
        }
        break;
    case UR_TYPE_UINT64:
        longval = PyLong_AsLongLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint64_t *) value) = (uint64_t) longval;
        }
        break;
    case UR_TYPE_INT8:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int8_t *) value) = (int8_t) longval;
        }
        break;
    case UR_TYPE_INT16:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int16_t *) value) = (int16_t) longval;
        }
        break;
    case UR_TYPE_INT32:
        longval = PyLong_AsLongLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int32_t *) value) = (int32_t) longval;
        }
        break;
    case UR_TYPE_INT64:
        longval = PyLong_AsLongLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int64_t *) value) = (int64_t) longval;
        }
        break;
    case UR_TYPE_CHAR:
        PyErr_SetString(PyExc_NotImplementedError, "Unknown UniRec field type.");
        return NULL;
        break;
    case UR_TYPE_FLOAT:
        floatval = PyFloat_AsDouble(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((float *) value) = (float) floatval;
        }
        break;
    case UR_TYPE_DOUBLE:
        floatval = PyFloat_AsDouble(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((double *) value) = floatval;
        }
        break;
    case UR_TYPE_IP:
        if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecIPAddr)) {
            pytrap_unirecipaddr *src = ((pytrap_unirecipaddr *) valueObj);
            ip_addr_t *dest = (ip_addr_t *) value;
            dest->ui64[0] = src->ip.ui64[0];
            dest->ui64[1] = src->ip.ui64[1];
        }
        break;
    case UR_TYPE_TIME:
        if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecTime)) {
            pytrap_unirectime *src = ((pytrap_unirectime *) valueObj);
            *((ur_time_t *) value) = src->timestamp;
        }
        break;
    case UR_TYPE_STRING:
        {
            Py_ssize_t size;
            char *str;
#if PY_MAJOR_VERSION >= 3
            if (!PyUnicode_Check(valueObj)) {
                PyErr_SetString(PyExc_TypeError, "String object expected.");
                return NULL;
            }
            str = PyUnicode_AsUTF8AndSize(valueObj, &size);
#else
            if (!PyString_Check(valueObj)) {
                PyErr_SetString(PyExc_TypeError, "String object expected.");
                return NULL;
            }
            if (PyString_AsStringAndSize(valueObj, &str, &size) == -1) {
                return NULL;
            }
#endif
            if (str != NULL) {
                /* TODO check return value */
                ur_set_var(self->urtmplt, data, field_id, str, size);
            }
        }
        break;
    case UR_TYPE_BYTES:
        {
            Py_ssize_t size;
            char *str;

            if (PyByteArray_Check(valueObj)) {
                size = PyByteArray_Size(valueObj);
                str = PyByteArray_AsString(valueObj);
            } else if (PyBytes_Check(valueObj)) {
                PyBytes_AsStringAndSize(valueObj, &str, &size);
            } else {
                PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
                return NULL;
            }

            if (str != NULL) {
                /* TODO check return value */
                ur_set_var(self->urtmplt, data, field_id, str, size);
            }
        }
        break;
    default:
        {
            PyErr_SetString(PyExc_TypeError, "Unknown UniRec field type.");
            return NULL;
        }
        break;
    } // case (field type)
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_getData(pytrap_unirectemplate *self)
{
    if (self->data_obj == NULL) {
        PyErr_SetString(TrapError, "No data was set - use setData before.");
        return NULL;
    }

    Py_INCREF(self->data_obj);
    return self->data_obj;
}

static PyObject *
UnirecTemplate_setByID(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    uint32_t field_id;
    PyObject *dataObj, *valueObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", "field_name", "value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "OIO", kwlist, &dataObj, &field_id, &valueObj)) {
        return NULL;
    }

    if (PyByteArray_Check(dataObj)) {
        data = PyByteArray_AsString(dataObj);
    } else if (PyBytes_Check(dataObj)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

    return UnirecTemplate_set_local(self, data, field_id, valueObj);
}

static PyObject *
UnirecTemplate_set(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    PyObject *dataObj, *valueObj, *field_name;
    int32_t field_id;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", "field_name", "value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "OOO", kwlist, &dataObj, &field_name, &valueObj)) {
        return NULL;
    }

    if (PyByteArray_Check(dataObj)) {
        data = PyByteArray_AsString(dataObj);
    } else if (PyBytes_Check(dataObj)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    if (!PyUnicode_Check(field_name))
#else
    if (!PyUnicode_Check(field_name) && !PyString_Check(field_name))
#endif
    {
        PyErr_SetString(PyExc_TypeError, "Argument field_name must be string.");
        return NULL;
    }

    field_id = UnirecTemplate_get_field_id(self, field_name);
    if (field_id == UR_ITER_END) {
        PyErr_SetString(TrapError, "Field was not found.");
        return NULL;
    }
    return UnirecTemplate_set_local(self, data, field_id, valueObj);
}

PyObject *
UnirecTemplate_getFieldsDict(pytrap_unirectemplate *self)
{
    PyObject *d = PyDict_New();
    PyObject *key;
    int i;

    if (d != NULL) {
        for (i = 0; i < self->urtmplt->count; i++) {
#if PY_MAJOR_VERSION >= 3
            key = PyUnicode_FromString(ur_get_name(self->urtmplt->ids[i]));
#else
            key = PyString_FromString(ur_get_name(self->urtmplt->ids[i]));
#endif
            PyDict_SetItem(d, key, PyLong_FromLong(self->urtmplt->ids[i]));
            Py_DECREF(key);
        }
        return d;
    }
    Py_DECREF(d);
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_setData(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds)
{
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &dataObj)) {
        return NULL;
    }

    if (PyByteArray_Check(dataObj)) {
        data_size = PyByteArray_Size(dataObj);
        data = PyByteArray_AsString(dataObj);
    } else if (PyBytes_Check(dataObj)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

    if (self->data != NULL) {
        /* decrease refCount of the previously stored data */
        Py_DECREF(self->data_obj);
    }
    self->data = data;
    self->data_size = data_size;

    self->data_obj = dataObj;
    /* Increment refCount for the original object, so it's not free'd */
    Py_INCREF(self->data_obj);

    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_createMessage(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds)
{
    char *data;
    uint32_t data_size = 0;

    static char *kwlist[] = {"dyn_size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|H", kwlist, &data_size)) {
        return NULL;
    }
    data_size += ur_rec_fixlen_size(self->urtmplt);
    if (data_size >= UR_MAX_SIZE) {
        PyErr_SetString(TrapError, "Max size of message is 65535 bytes.");
        return NULL;
    }
    data = ur_create_record(self->urtmplt, (uint16_t) data_size);
    PyObject *res = PyByteArray_FromStringAndSize(data, (uint16_t) data_size);

    if (self->data != NULL) {
        /* decrease refCount of the previously stored data */
        Py_DECREF(self->data_obj);
    }
    self->data_obj = res;
    self->data_size = PyByteArray_Size(res);
    self->data = PyByteArray_AsString(res);
    /* Increment refCount for the original object, so it's not free'd */
    Py_INCREF(self->data_obj);
    free(data);

    return res;
}

static PyTypeObject pytrap_UnirecTemplate;

static pytrap_unirectemplate *
UnirecTemplate_init(pytrap_unirectemplate *self)
{
    self->data = NULL;
    self->data_size = 0;
    self->data_obj = NULL;
    self->urdict = (PyDictObject *) UnirecTemplate_getFieldsDict(self);

    self->iter_index = 0;
    self->field_count = PyDict_Size((PyObject *) self->urdict);
    return self;
}

static PyObject *
UnirecTemplate_copy(pytrap_unirectemplate *self)
{
    pytrap_unirectemplate *n;
    n = (pytrap_unirectemplate *) pytrap_UnirecTemplate.tp_alloc(&pytrap_UnirecTemplate, 0);

    char *s = ur_template_string_delimiter(self->urtmplt, ',');
    n->urtmplt = ur_create_template_from_ifc_spec(s);
    free(s);
    if (n->urtmplt == NULL) {
        PyErr_SetString(TrapError, "Creation of UniRec template failed.");
        Py_DECREF(n);
        return NULL;
    }

    n = UnirecTemplate_init(n);

    return (PyObject *) n;
}

static PyObject *
UnirecTemplate_strRecord(pytrap_unirectemplate *self)
{
    if (self->data == NULL) {
        PyErr_SetString(TrapError, "Data was not set yet.");
        return NULL;
    }

    PyObject *l = PyList_New(0);
    PyObject *i;
    PyObject *format = PyUnicode_FromString("format");
    PyObject *keyval;
    PyObject *val;

    ur_field_id_t id = UR_ITER_BEGIN;
    while ((id = ur_iter_fields(self->urtmplt, id)) != UR_ITER_END) {
        i = PyUnicode_FromFormat("%s = {0!r}", ur_get_name(id), "value");
        val = UnirecTemplate_get_local(self, self->data, id);
        keyval =  PyObject_CallMethodObjArgs(i, format, val, NULL);
        PyList_Append(l, keyval);
        Py_DECREF(i);
        Py_DECREF(val);
        Py_XDECREF(keyval);
    }
    PyObject *delim = PyUnicode_FromString(", ");
    PyObject *join = PyUnicode_FromString("join");
    PyObject *result =  PyObject_CallMethodObjArgs(delim, join, l, NULL);
    Py_DECREF(delim);
    Py_DECREF(join);
    Py_DECREF(format);
    Py_DECREF(l);
    return result;
}

static PyObject *
UnirecTemplate_getFieldType(pytrap_unirectemplate *self, PyObject *args)
{
    PyObject *name;

    if (!PyArg_ParseTuple(args, "O", &name)) {
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    if (!PyUnicode_Check(name))
#else
    if (!PyUnicode_Check(name) && !PyString_Check(name))
#endif
    {
        PyErr_SetString(PyExc_TypeError, "Argument field_name must be string.");
        return NULL;
    }
    int32_t field_id = UnirecTemplate_get_field_id(self, name);

    switch (ur_get_type(field_id)) {
    case UR_TYPE_UINT8:
    case UR_TYPE_UINT16:
    case UR_TYPE_UINT32:
    case UR_TYPE_UINT64:
    case UR_TYPE_INT8:
    case UR_TYPE_INT16:
    case UR_TYPE_INT32:
    case UR_TYPE_INT64:
    case UR_TYPE_CHAR:
        return (PyObject *) &PyLong_Type;
        break;
    case UR_TYPE_FLOAT:
    case UR_TYPE_DOUBLE:
        return (PyObject *) &PyFloat_Type;
        break;
    case UR_TYPE_IP:
        return (PyObject *) &pytrap_UnirecIPAddr;
    case UR_TYPE_TIME:
        return (PyObject *) &pytrap_UnirecTime;
    case UR_TYPE_STRING:
#if PY_MAJOR_VERSION >= 3
        return (PyObject *) &PyUnicode_Type;
#else
        return (PyObject *) &PyString_Type;
#endif
        break;
    case UR_TYPE_BYTES:
        return (PyObject *) &PyByteArray_Type;
        break;
    default:
        PyErr_SetString(PyExc_NotImplementedError, "Unknown UniRec field type.");
        return NULL;
    } // case (field type)
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_recFixlenSize(pytrap_unirectemplate *self)
{
    uint16_t rec_size = ur_rec_fixlen_size(self->urtmplt);
    return Py_BuildValue("H", rec_size);
}

static PyObject *
UnirecTemplate_recVarlenSize(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    PyObject *dataObj = NULL;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|O", kwlist, &dataObj)) {
        return NULL;
    }

    if (dataObj != NULL) {
        if (PyByteArray_Check(dataObj)) {
            //data_size = PyByteArray_Size(dataObj);
            data = PyByteArray_AsString(dataObj);
        } else if (PyBytes_Check(dataObj)) {
            PyBytes_AsStringAndSize(dataObj, &data, &data_size);
        } else {
            PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
            return NULL;
        }
    } else {
        if (self->data != NULL) {
            data = self->data;
        } else {
            PyErr_SetString(PyExc_TypeError, "Data was not set nor expolicitly passed as argument.");
            return NULL;
        }
    }

    uint16_t rec_size = ur_rec_varlen_size(self->urtmplt, data);
    return Py_BuildValue("H", rec_size);
}

static PyObject *
UnirecTemplate_recSize(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    PyObject *dataObj = NULL;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|O", kwlist, &dataObj)) {
        return NULL;
    }

    if (dataObj != NULL) {
        if (PyByteArray_Check(dataObj)) {
            //data_size = PyByteArray_Size(dataObj);
            data = PyByteArray_AsString(dataObj);
        } else if (PyBytes_Check(dataObj)) {
            PyBytes_AsStringAndSize(dataObj, &data, &data_size);
        } else {
            PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
            return NULL;
        }
    } else {
        if (self->data != NULL) {
            data = self->data;
        } else {
            PyErr_SetString(PyExc_TypeError, "Data was not set nor expolicitly passed as argument.");
            return NULL;
        }
    }

    uint16_t rec_size = ur_rec_size(self->urtmplt, data);
    return Py_BuildValue("H", rec_size);
}

static PyMethodDef pytrap_unirectemplate_methods[] = {
        {"getFieldType", (PyCFunction) UnirecTemplate_getFieldType, METH_VARARGS,
            "Get type of given field.\n\n"
            "Args:\n"
            "    field_name (str): Field name.\n\n"
            "Returns:\n"
            "    type: Type object (e.g. int, str or pytrap.UnirecIPAddr).\n"
        },

        {"getByID", (PyCFunction) UnirecTemplate_getByID, METH_VARARGS | METH_KEYWORDS,
            "Get value of the field from the UniRec message.\n\n"
            "Args:\n"
            "    data (bytearray or bytes): Data - UniRec message.\n"
            "    field_id (int): Field ID (use getFieldsDict()).\n"
            "Returns:\n"
            "    (object): Retrieved value of the field (depends on UniRec template).\n\n"
            "Raises:\n"
            "    TypeError: Data argument must be bytearray or bytes.\n"
        },

        {"get", (PyCFunction) UnirecTemplate_getByName, METH_VARARGS | METH_KEYWORDS,
            "Get value of the field from the UniRec message.\n\n"
            "Args:\n"
            "    data (bytearray or bytes): Data - UniRec message.\n"
            "    field_name (str): Field name.\n"
            "Returns:\n"
            "    (object): Retrieved value of the field (depends on UniRec template).\n\n"
            "Raises:\n"
            "    TypeError: Data argument must be bytearray or bytes.\n"
            "    TrapError: Field name was not found.\n"
        },

        {"setByID", (PyCFunction) UnirecTemplate_setByID, METH_VARARGS | METH_KEYWORDS,
            "Set value of the field in the UniRec message.\n\n"
            "Args:\n"
            "    data (bytearray or bytes): Data - UniRec message.\n"
            "    field_id (int): Field ID.\n"
            "    value (object): New value of the field (depends on UniRec template).\n\n"
            "Raises:\n"
            "    TypeError: Bad object type of value was given.\n"
            "    TrapError: Field was not found.\n"
        },

        {"set", (PyCFunction) UnirecTemplate_set, METH_VARARGS | METH_KEYWORDS,
            "Set value of the field in the UniRec message.\n\n"
            "Args:\n"
            "    data (bytearray or bytes): Data - UniRec message.\n"
            "    field_name (str): Field name.\n"
            "    value (object): New value of the field (depends on UniRec template).\n\n"
            "Raises:\n"
            "    TypeError: Bad object type of value was given.\n"
            "    TrapError: Field was not found.\n"
        },

        {"getData", (PyCFunction) UnirecTemplate_getData, METH_NOARGS,
            "Get data that was already set using setData.\n\n"
            "Returns:\n"
            "    bytearray: Data - UniRec message.\n"
        },

        {"setData", (PyCFunction) UnirecTemplate_setData, METH_VARARGS | METH_KEYWORDS,
            "Set data for attribute access.\n\n"
            "Args:\n"
            "    data (bytearray or bytes): Data - UniRec message.\n"
        },

        {"getFieldsDict", (PyCFunction) UnirecTemplate_getFieldsDict, METH_NOARGS,
            "Get set of fields of the template.\n\n"
            "Returns:\n"
            "    Dict(str,int): Dictionary of field_id with field name as a key.\n"
        },

        {"createMessage", (PyCFunction) UnirecTemplate_createMessage, METH_VARARGS | METH_KEYWORDS,
            "Create a message that can be filled in with values according to the template.\n\n"
            "Args:\n"
            "    dyn_size (Optional[int]): Maximal size of variable data (in total) (default: 0).\n\n"
            "Returns:\n"
            "    bytearray: Allocated memory that can be filled in using set().\n"
        },

        {"copy", (PyCFunction) UnirecTemplate_copy, METH_NOARGS,
            "Create a new instance with the same format specifier without data.\n\n"
            "Returns:\n"
            "    UnirecTemplate: New copy of object (not just reference).\n"
        },

        {"strRecord", (PyCFunction) UnirecTemplate_strRecord, METH_NOARGS,
            "Get values of record in readable format.\n\n"
            "Returns:\n"
            "    str: String in key=value format.\n"
        },

        {"recSize", (PyCFunction) UnirecTemplate_recSize, METH_VARARGS | METH_KEYWORDS,
            "Get size of UniRec record.\n\n"
            "Return total size of valid record data, i.e. number of bytes occupied by all fields."
            "This may be less than allocated size of 'data'.\n"
            "Args:\n"
            "    data (Optional[bytearray or bytes]): Data of UniRec message (optional if previously set by setData)"
            "Returns:\n"
            "    int: Size of record data in bytes\n"
        },

        {"recFixlenSize", (PyCFunction) UnirecTemplate_recFixlenSize, METH_NOARGS,
            "Get size of fixed part of UniRec record.\n\n"
            "This is the minimal size of a record with given template,"
            "i.e. the size with all variable-length fields empty.\n"
            "Returns:\n"
            "    int: Size of fixed part of template in bytes\n"
        },


        {"recVarlenSize", (PyCFunction) UnirecTemplate_recVarlenSize, METH_VARARGS | METH_KEYWORDS,
            "Get size of variable-length part of UniRec record.\n\n"
            "Return total size of all variable-length fields.\n"
            "Args:\n"
            "    data (Optional[bytearray or bytes]): Data of UniRec message (optional if previously set by setData)"
            "Returns:\n"
            "    int: Total size of variable-length fields in bytes\n"
        },

        {NULL, NULL, 0, NULL}
};

static PyObject *
UnirecTemplate_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirectemplate *self;
    const char *spec;
    // TODO return error string of failure during unirec template init
    //char *errstring;

    self = (pytrap_unirectemplate *) type->tp_alloc(type, 0);
    if (self != NULL) {
        static char *kwlist[] = {"spec", NULL};
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &spec)) {
            Py_DECREF(self);
            return NULL;
        }
        self->urtmplt = NULL;
        int ret;
        if ((ret = ur_define_set_of_fields(spec)) != UR_OK) {
            PyErr_SetString(TrapError, "ur_define_set_of_fields() failed.");
            Py_DECREF(self);
            return NULL;
        }
        /* XXX errstring */
        //self->urtmplt = ur_create_template(spec, &errstring);
        self->urtmplt = ur_create_template_from_ifc_spec(spec);
        if (self->urtmplt == NULL) {
            //PyErr_SetString(TrapError, errstring);
            PyErr_SetString(TrapError, "Creation of UniRec template failed.");
            //free(errstring);
            Py_DECREF(self);
            return NULL;
        }
        self = UnirecTemplate_init(self);
    }

    return (PyObject *) self;
}

static void UnirecTemplate_dealloc(pytrap_unirectemplate *self)
{
    if (self->urdict) {
        Py_DECREF(self->urdict);
    }
    if (self->urtmplt) {
        ur_free_template(self->urtmplt);
    }
    Py_XDECREF(self->data_obj); // Allow to free the original data object
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *
UnirecTemplate_str(pytrap_unirectemplate *self)
{
    char *s = ur_template_string_delimiter(self->urtmplt, ',');
    PyObject *result;
#if PY_MAJOR_VERSION >= 3
    result = PyUnicode_FromFormat("(%s)", s);
#else
    result = PyString_FromFormat("(%s)", s);
#endif
    free(s);
    return result;
}

static PyObject *
UnirecTemplate_getAttr(pytrap_unirectemplate *self, PyObject *attr)
{
    int32_t field_id = UnirecTemplate_get_field_id(self, attr);

    if (field_id == UR_ITER_END) {
        return PyObject_GenericGetAttr((PyObject *) self, attr);
    }

    return UnirecTemplate_get_local(self, self->data, field_id);
}

static int
UnirecTemplate_setAttr(pytrap_unirectemplate *self, PyObject *attr, PyObject *value)
{
    PyObject *v = PyDict_GetItem((PyObject *) self->urdict, attr);
    if (v == NULL) {
        return PyObject_GenericSetAttr((PyObject *) self, attr, value);
    }
    int32_t field_id;
    field_id = (int32_t) PyLong_AsLong(v);

    if (UnirecTemplate_set_local(self, self->data, field_id, value) == NULL) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

Py_ssize_t UnirecTemplate_len(pytrap_unirectemplate *o)
{
   return o->field_count;
}

static PyObject *
UnirecTemplate_next(pytrap_unirectemplate *self)
{
    PyObject *name;
    PyObject *value;

    if (self->iter_index < self->field_count) {
#if PY_MAJOR_VERSION >= 3
        name = PyUnicode_FromString(ur_get_name(self->urtmplt->ids[self->iter_index]));
#else
        name = PyString_FromString(ur_get_name(self->urtmplt->ids[self->iter_index]));
#endif
        value = UnirecTemplate_get_local(self, self->data, self->urtmplt->ids[self->iter_index]);
        self->iter_index++;
        return Py_BuildValue("(OO)", name, value);
    }

    self->iter_index = 0;
    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
}

static PySequenceMethods UnirecTemplate_seqmethods = {
    (lenfunc) UnirecTemplate_len, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    0, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
};

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
    &UnirecTemplate_seqmethods,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecTemplate_str,                         /* tp_str */
    (getattrofunc) UnirecTemplate_getAttr,                         /* tp_getattro */
    (setattrofunc) UnirecTemplate_setAttr,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Class for UniRec template storage and base data access.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc) UnirecTemplate_next,                         /* tp_iternext */
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


/*************************/
/*    UnirecIPAddrRange  */
/*************************/

static PyTypeObject pytrap_UnirecIPAddrRange;

typedef struct {
    PyObject_HEAD
    pytrap_unirecipaddr *start; /* low ip */
    pytrap_unirecipaddr *end;   /* high ip*/
} pytrap_unirecipaddrrange;

static void
UnirecIPAddrRange_dealloc(pytrap_unirecipaddrrange *self)
{
    Py_XDECREF(self->start);
    Py_XDECREF(self->end);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
UnirecIPAddrRange_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirecipaddrrange *self;

    self = (pytrap_unirecipaddrrange *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->start = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);

        if (self->start == NULL) {
            //  Py_DECREF(self);
            return NULL;
        }
        Py_INCREF(self->start);

        self->end = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);

        if (self->end == NULL) {
            // Py_DECREF(self);
            return NULL;
        }
        Py_INCREF(self->end);
    }

    return (PyObject *)self;
}


static PyObject *
UnirecIPAddrRange_isIn(pytrap_unirecipaddrrange *self, PyObject *args)
{
    pytrap_unirecipaddr *ipaddr = (pytrap_unirecipaddr *) args;
    PyObject *result = Py_False;

    if (!PyObject_IsInstance(args, (PyObject *) &pytrap_UnirecIPAddr)) {
        result = Py_NotImplemented;
    }

    int cmp_result;
    cmp_result = ip_cmp(&self->start->ip, &ipaddr->ip);

    if (cmp_result == 0) {
        /* ip address is in interval */
        result = PyLong_FromLong(0);
    } else if (cmp_result > 0) {
        /* ip address is lower then interval */
        result = PyLong_FromLong(-1);
    } else {
        cmp_result = ip_cmp(&self->end->ip, &ipaddr->ip);
        if (cmp_result >= 0) {
            /* ip address is in interval */
            result = PyLong_FromLong(0);
        } else {
            /* ip address is greater then interval */
            result = PyLong_FromLong(1);
        }
    }

    Py_INCREF(result);
    return result;
}


static PyObject *
UnirecIPAddrRange_isOverlap(pytrap_unirecipaddrrange *self, PyObject *args)
{
    /* compared ranges must by sorted by low ip and mask */
    pytrap_unirecipaddrrange *other;

    PyObject * tmp;
    long cmp_result;

    if (!PyArg_ParseTuple(args, "O", &other))
        return NULL;

    if (!PyObject_IsInstance((PyObject*)other, (PyObject *) &pytrap_UnirecIPAddrRange)) {
        return Py_NotImplemented;
    }

    tmp = UnirecIPAddrRange_isIn(self, (PyObject *) other->start);
    cmp_result = PyLong_AsLong(tmp);
    Py_DECREF(tmp);

    if (cmp_result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static int
UnirecIPAddrRange_contains(pytrap_unirecipaddrrange *o, pytrap_unirecipaddr *ip)
{
    PyObject * tmp = UnirecIPAddrRange_isIn(o, (PyObject *) ip);
    int cmp_result = PyLong_AsLong(tmp);
    Py_DECREF(tmp);
    return cmp_result == 0 ? 1 : 0;
}

static uint8_t
bit_endian_swap(uint8_t in)
{
    in = (in & 0xF0) >> 4 | (in & 0x0F) << 4;
    in = (in & 0xCC) >> 2 | (in & 0x33) << 2;
    in = (in & 0xAA) >> 1 | (in & 0x55) << 1;
    return in;
}

static int
UnirecIPAddrRange_init(pytrap_unirecipaddrrange *self, PyObject *args, PyObject *kwds)
{
    PyObject *start = NULL, *end = NULL;
    char *start_str, *end_str;
    Py_ssize_t size;

    static char *kwlist[] = {"start", "end", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &start, &end)) {
        return -1;
    }

    if (end) {
        if (PyObject_IsInstance(end, (PyObject *) &pytrap_UnirecIPAddr)) {
            pytrap_unirecipaddr *src = ((pytrap_unirecipaddr *) end);
            memcpy(&self->end->ip, &src->ip, 16);
        } else {
#if PY_MAJOR_VERSION >= 3
            if (!PyUnicode_Check(end)) {
                PyErr_SetString(PyExc_TypeError, "String or UnirecIPAddr object expected.");
                return -1;
            } else {
                end_str = PyUnicode_AsUTF8AndSize(end, &size);
            }
#else
            if (!PyString_Check(end)) {
                PyErr_SetString(PyExc_TypeError, "String or UnirecIPAddr object expected.");
                return -1;
            }
            if (PyString_AsStringAndSize(end, &end_str, &size) == -1) {
                return -1;
            }
#endif
            pytrap_UnirecIPAddr.tp_init((PyObject *) self->end, Py_BuildValue("(s)", end_str), kwds);
        }
    }

    if (start) {
        if (PyObject_IsInstance(start, (PyObject *) &pytrap_UnirecIPAddr)) {
            pytrap_unirecipaddr *src = ((pytrap_unirecipaddr *) start);
            memcpy(&self->start->ip, &src->ip,16 );

            if (!end) {
                PyErr_SetString(PyExc_TypeError, "Missing end IP address");
                return -1;
            }
        } else {
#if PY_MAJOR_VERSION >= 3
            if (!PyUnicode_Check(start)) {
                PyErr_SetString(PyExc_TypeError, "String or UnirecIPAddr object expected.");
                return -1;
            } else {
                start_str = PyUnicode_AsUTF8AndSize(start, &size);
            }
#else
            if (!PyString_Check(start)) {
                PyErr_SetString(PyExc_TypeError, "String or UnirecIPAddr object expected.");
                return -1;
            }
            if (PyString_AsStringAndSize(start, &start_str, &size) == -1) {
                return -1;
            }
#endif
            if (end) {
                pytrap_UnirecIPAddr.tp_init((PyObject *)self->start,  Py_BuildValue("(s)", start_str), kwds);
            } else {
                char * str;
                long mask;
                ip_addr_t tmp_ip;
                Py_ssize_t size;
                PyObject *net;

#if PY_MAJOR_VERSION >= 3
                net = start;
#else
                net = PyUnicode_FromObject(start);
#endif
                PyObject *sep = PyUnicode_FromString("/");
                PyObject *par = PyUnicode_Split(net, sep, 1);

                if (PyList_Size(par) == 1 && !end) {
                    PyErr_SetString(PyExc_TypeError, "Missing end IP.");
                    return -1;
                }

#if PY_MAJOR_VERSION >= 3
                if (!PyUnicode_Check(start)) {
                    PyErr_SetString(PyExc_TypeError, "String object expected.");
                    return -1;
                }

                str = PyUnicode_AsUTF8AndSize(PyList_GetItem(par, 0), &size);
                PyObject *py_mask = PyLong_FromUnicodeObject(PyList_GetItem(par, 1), 10);
                mask = PyLong_AsLong(py_mask);
                Py_DECREF(py_mask);
#else
                if (!PyString_Check(start)) {
                    PyErr_SetString(PyExc_TypeError, "String object expected");
                    return -1;
                }

                if (PyString_AsStringAndSize(PyList_GetItem(par, 0), &str, &size) == -1) {
                    return -1;
                }

                char * msk = NULL;
                if (PyString_AsStringAndSize(PyList_GetItem(par, 1), &msk, &size) == -1) {
                    return -1;
                }

                mask = PyLong_AsLong(PyLong_FromString(msk, NULL, 10));
#endif
                Py_DECREF(sep);
                Py_DECREF(par);

                if (ip_from_str(str, &tmp_ip) != 1) {
                    PyErr_SetString(TrapError, "Could not parse given IP address.");
                    return -1;
                }

                if (ip_is4(&tmp_ip)) {
                    uint32_t mask_bit;
                    mask_bit = 0xFFFFFFFF>>(mask > 31 ? 0 : 32 - mask);
                    mask_bit = (bit_endian_swap((mask_bit & 0x000000FF)>>  0) <<  0) |
                        (bit_endian_swap((mask_bit & 0x0000FF00)>>  8) <<  8) |
                        (bit_endian_swap((mask_bit & 0x00FF0000)>> 16) << 16) |
                        (bit_endian_swap((mask_bit & 0xFF000000)>> 24) << 24);

                    tmp_ip.ui32[2] = tmp_ip.ui32[2] & mask_bit;
                    memcpy( &self->start->ip, &tmp_ip, 16);

                    tmp_ip.ui32[2] = tmp_ip.ui32[2] | (~ mask_bit);
                    memcpy( &self->end->ip, &tmp_ip, 16);

                } else {
                    uint32_t * net_mask_array;

                    net_mask_array = malloc(4 * sizeof(uint32_t));
                    if (net_mask_array == NULL) {
                        return -1;
                    }
                    // Fill every word of IPv6 address
                    net_mask_array[0] = 0xFFFFFFFF>>(mask > 31 ? 0 : 32 - mask);
                    net_mask_array[1] = 0xFFFFFFFF>>(mask > 63 ? 0 : (mask > 32 ? 64 - mask: 32));
                    net_mask_array[2] = 0xFFFFFFFF>>(mask > 95 ? 0 : (mask > 64 ? 96 - mask: 32));
                    net_mask_array[3] = 0xFFFFFFFF>>(mask > 127 ? 0 :(mask > 96 ? 128 - mask : 32));

                    int i;

                    // Swap bits in every byte for compatibility with ip_addr_t stucture
                    for (i = 0; i < 4; ++i) {
                        net_mask_array[i] = (bit_endian_swap((net_mask_array[i] & 0x000000FF)>>  0) <<  0) |
                            (bit_endian_swap((net_mask_array[i] & 0x0000FF00)>>  8) <<  8) |
                            (bit_endian_swap((net_mask_array[i] & 0x00FF0000)>> 16) << 16) |
                            (bit_endian_swap((net_mask_array[i] & 0xFF000000)>> 24) << 24);
                    }

                    for (i = 0; i < 4; i++) {
                        tmp_ip.ui32[i] = tmp_ip.ui32[i] & net_mask_array[i];
                    }
                    memcpy( &self->start->ip, &tmp_ip, 16);

                    for (i = 0; i < 4; i++) {
                        tmp_ip.ui32[i] = tmp_ip.ui32[i] | ( ~ net_mask_array[i]);
                    }
                    memcpy( &self->end->ip, &tmp_ip, 16);

                    free(net_mask_array);
                }
            }
        }
    }
    return 0;
}

static PyObject *
UnirecIPAddrRange_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecIPAddrRange) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecIPAddrRange)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirecipaddrrange *ur_a = (pytrap_unirecipaddrrange *) a;
    pytrap_unirecipaddrrange *ur_b = (pytrap_unirecipaddrrange *) b;

    int res_S = ip_cmp(&ur_a->start->ip, &ur_b->start->ip);
    int res_E = ip_cmp(&ur_a->end->ip, &ur_b->end->ip);

    switch (op) {
    case Py_EQ:
        result = (res_S == 0 && res_E == 0? Py_True : Py_False);
        break;
    case Py_NE:
        result = (res_S != 0 && res_E != 0 ? Py_True : Py_False);
        break;
    case Py_LE:
        result = (res_S <= 0 ? Py_True : Py_False);
        break;
    case Py_GE:
        result = (res_E >= 0 ? Py_True : Py_False);
        break;
    case Py_LT:
        result = (res_S < 0 ? Py_True : Py_False);
        break;
    case Py_GT:
        result = (res_E > 0 ? Py_True : Py_False);
        break;
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyObject *
UnirecIPAddrRange_repr(pytrap_unirecipaddrrange *self)
{
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("UnirecIPAddrRange('%U' - '%U')", UnirecIPAddr_str(self->start), UnirecIPAddr_str(self->end));
#else
    return PyString_FromFormat("UnirecIPAddrRange('%s' - '%s')", PyString_AsString(UnirecIPAddr_str(self->start)),
        PyString_AsString(UnirecIPAddr_str(self->end)));
#endif
}

static PyObject *
UnirecIPAddrRange_str(pytrap_unirecipaddrrange *self)
{
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("%U - %U", UnirecIPAddr_str(self->start), UnirecIPAddr_str(self->end));
#else
    return PyString_FromFormat("%s - %s", PyString_AsString(UnirecIPAddr_str(self->start)),
        PyString_AsString(UnirecIPAddr_str(self->end)));
#endif
}

static PyMemberDef UnirecIPAddrRange_members[] = {
    {"start", T_OBJECT_EX, offsetof(pytrap_unirecipaddrrange, start), 0,
     "Low IP address of range"},
    {"end", T_OBJECT_EX, offsetof(pytrap_unirecipaddrrange, end), 0,
     "High IP address of range"},
    {NULL}  /* Sentinel */
};


static PyMethodDef UnirecIPAddrRange_methods[] = {
    {"isIn", (PyCFunction) UnirecIPAddrRange_isIn, METH_O,
        "Check if the address is in IP range.\n\n"
        "Args:\n"
        "    ipaddr: UnirecIPAddr struct"
        "Returns:\n"
        "    long: -1 if ipaddr < start of range, 0 if ipaddr is in interval, 1 if ipaddr > end of range .\n"
        },

    {"isOverlap", (PyCFunction) UnirecIPAddrRange_isOverlap, METH_VARARGS,
        "Check if 2 ranges sorted by start IP overlap.\n\n"
        "Args:\n"
        "    other: UnirecIPAddrRange, second interval to compare"
        "Returns:\n"
        "    bool: True if ranges are overlaps.\n"
        },
    {NULL}  /* Sentinel */
};


static PySequenceMethods UnirecIPAddrRange_seqmethods = {
    0, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    (objobjproc) UnirecIPAddrRange_contains, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
};


static PyTypeObject pytrap_UnirecIPAddrRange = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecIPAddrRange",       /* tp_name */
    sizeof(pytrap_unirecipaddrrange), /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) UnirecIPAddrRange_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    (reprfunc) UnirecIPAddrRange_repr, /* tp_repr */
    0,                         /* tp_as_number */
    &UnirecIPAddrRange_seqmethods, /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecIPAddrRange_str, /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Unirec IPAddr Range object",   /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc) UnirecIPAddrRange_compare, /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    UnirecIPAddrRange_methods, /* tp_methods */
    UnirecIPAddrRange_members, /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)UnirecIPAddrRange_init,      /* tp_init */
    0,                         /* tp_alloc */
    UnirecIPAddrRange_new,     /* tp_new */
};



/**
 * \brief Initialize UniRec template class and add it to pytrap module.
 *
 * \param [in,out] m    pointer to the module Object
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int
init_unirectemplate(PyObject *m)
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

    /* Add IPAddrRange */
    if (PyType_Ready(&pytrap_UnirecIPAddrRange) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecIPAddrRange);
    PyModule_AddObject(m, "UnirecIPAddrRange", (PyObject *) &pytrap_UnirecIPAddrRange);

    /* Add Template */
    if (PyType_Ready(&pytrap_UnirecTemplate) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecTemplate);
    PyModule_AddObject(m, "UnirecTemplate", (PyObject *) &pytrap_UnirecTemplate);

    PyDateTime_IMPORT;

    return EXIT_SUCCESS;
}


