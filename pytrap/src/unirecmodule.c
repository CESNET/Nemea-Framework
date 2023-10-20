#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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
#include "unirectemplate.h"
#include "unirecipaddr.h"
#include "unirecmacaddr.h"
#include "pytrapexceptions.h"

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
UnirecTime_getMicroSeconds(pytrap_unirectime *self)
{
    return PyLong_FromLong(ur_time_get_usec(self->timestamp));
}

static PyObject *
UnirecTime_getMicroSecondsAsFloat(pytrap_unirectime *self)
{
    double t = (double)ur_time_get_sec(self->timestamp)*1000000;
    t += (double) ur_time_get_usec(self->timestamp);
    return PyFloat_FromDouble(t);
}

static PyObject *
UnirecTime_getNanoSeconds(pytrap_unirectime *self)
{
    return PyLong_FromLong(ur_time_get_nsec(self->timestamp));
}

static PyObject *
UnirecTime_getNanoSecondsAsFloat(pytrap_unirectime *self)
{
    double t = (double)ur_time_get_sec(self->timestamp)*1000000000;
    t += (double) ur_time_get_nsec(self->timestamp);
    return PyFloat_FromDouble(t);
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

        /* since reference counter is not increased for fmt by
         * PyArg_ParseTuple(), we must increment manually, so we can decrement
         * after PyObject_CallMethodObjArgs() for all cases */
        Py_INCREF(fmt);
    } else {
        fmt = PyUnicode_FromString("%FT%TZ");
    }
    PyObject *strftime = PyUnicode_FromString("strftime");

    PyObject *result = PyObject_CallMethodObjArgs(dt, strftime, fmt, NULL);
    Py_DECREF(fmt);
    Py_DECREF(dt);
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
    {"getMicroSeconds", (PyCFunction) UnirecTime_getMicroSeconds, METH_NOARGS,
        "Get number of microseconds of timestamp.\n\n"
        "Returns:\n"
        "    (long): Retrieved number of microseconds.\n"
    },
    {"getMicroSecondsAsFloat", (PyCFunction) UnirecTime_getMicroSecondsAsFloat, METH_NOARGS,
        "Get number of microseconds of timestamp.\n\n"
        "Returns:\n"
        "    (double): Retrieved number of microseconds.\n"
    },
    {"getNanoSeconds", (PyCFunction) UnirecTime_getNanoSeconds, METH_NOARGS,
        "Get number of nanoseconds of timestamp.\n\n"
        "Returns:\n"
        "    (long): Retrieved number of nanoseconds.\n"
    },
    {"getNanoSecondsAsFloat", (PyCFunction) UnirecTime_getNanoSecondsAsFloat, METH_NOARGS,
        "Get number of nanoseconds of timestamp.\n\n"
        "Returns:\n"
        "    (double): Retrieved number of nanoseconds.\n"
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
    PyObject *arg1;
    uint32_t secs = 0, msecs = 0;
    double fl_time;
    Py_ssize_t csize;
#if PY_MAJOR_VERSION >= 3
    const char *cstr = NULL;
#else
    char *cstr = NULL;
#endif

    if (s != NULL) {
        if (!PyArg_ParseTuple(args, "O|I", &arg1, &msecs)) {
            return -1;
        }
        if (PyFloat_Check(arg1)) {
            fl_time = PyFloat_AsDouble(arg1);
            secs = (uint32_t) fl_time;
            msecs = (uint32_t) (1000 * (fl_time - secs));
        } else if (PyLong_Check(arg1)) {
            secs = (uint32_t) PyLong_AsLong(arg1);
#if PY_MAJOR_VERSION < 3
        } else if (PyInt_Check(arg1)) {
            secs = (uint32_t) PyInt_AsLong(arg1);
#endif
#if PY_MAJOR_VERSION >= 3
        } else if (PyUnicode_Check(arg1)) {
            cstr = PyUnicode_AsUTF8AndSize(arg1, &csize);
#else
        } else if (PyString_Check(arg1)) {
            if (PyString_AsStringAndSize(arg1, &cstr, &csize) == -1) {
                return -1;
            }
#endif
        } else {
           PyErr_SetString(PyExc_TypeError, "Unsupported argument type.");
           return -1;
        }
        if (cstr != NULL) {
            if (ur_time_from_string(&s->timestamp, cstr) == 0) {
                return 0;
            } else {
                PyErr_SetString(PyExc_TypeError, "Malformed string argument, YYYY-mm-ddTHH:MM:SS expected.");
                return -1;
            }
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
        Py_TPFLAGS_BASETYPE, /* tp_flags */
    "UnirecTime(int(seconds), [int(miliseconds)])\n"
    "UnirecTime(double(secs_and_msecs))\n"
    "UnirecTime(str(\"2019-03-18T12:11:10Z\"))\n\n"
    "    Class for UniRec timestamp storage and base data access.\n\n"
    "    Args:\n"
    "        str: datetime, e.g., \"2019-03-18T12:11:10.123Z\"\n"
    "        double or int: number of seconds\n"
    "        Optional[int]: number of miliseconds (when the first argument is int)\n\n"
    "    Raises:\n"
    "        TypeError: unsupported type was provided or string is malformed.\n", /* tp_doc */
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
/* UnirecTemplate    */
/*********************/


static inline PyObject *
UnirecTemplate_get_local(pytrap_unirectemplate *self, char *data, int32_t field_id)
{
    if (data == NULL) {
        PyErr_SetString(TrapError, "Data was not set yet.");
        return NULL;
    }
    int type = ur_get_type(field_id);
    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);
    int array_len = 0;
    int i;
    PyObject *list = NULL, *elem = NULL;

    if (ur_is_varlen(field_id) && type != UR_TYPE_STRING && type != UR_TYPE_BYTES) {
       list = PyList_New(0);
       array_len = ur_array_get_elem_cnt(self->urtmplt, data, field_id);
    }

    switch (type) {
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
        break;
    case UR_TYPE_MAC:
        {
            pytrap_unirecmacaddr *new_mac = (pytrap_unirecmacaddr *) pytrap_UnirecMACAddr.tp_alloc(&pytrap_UnirecMACAddr, 0);
            memcpy(&new_mac->mac, value, sizeof(mac_addr_t));
            return (PyObject *) new_mac;
        }
        break;
    case UR_TYPE_TIME:
        {
            pytrap_unirectime *new_time = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
            new_time->timestamp = *((ur_time_t *) value);
            return (PyObject *) new_time;
        }
        break;
    case UR_TYPE_STRING:
        {
            Py_ssize_t value_size = ur_get_var_len(self->urtmplt, data, field_id);
            return PyUnicode_DecodeUTF8(value, value_size, "replace");
        }
        break;
    case UR_TYPE_BYTES:
        {
            Py_ssize_t value_size = ur_get_var_len(self->urtmplt, data, field_id);
            return PyByteArray_FromStringAndSize(value, value_size);
        }
        break;
    case UR_TYPE_A_UINT8:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("B", ((uint8_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_INT8:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("b", ((int8_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_UINT16:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("H", ((uint16_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_INT16:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("h", ((int16_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_UINT32:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("I", ((uint32_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_INT32:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("i", ((int32_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_UINT64:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("K", ((uint64_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_INT64:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("L", ((int64_t *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_FLOAT:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("f", ((float *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_DOUBLE:
         for (i = 0; i < array_len; i++) {
            elem = Py_BuildValue("d", ((double *) value)[i]);
            PyList_Append(list, elem);
            Py_DECREF(elem);
         }
         return list;
    case UR_TYPE_A_IP:
         for (i = 0; i < array_len; i++) {
            pytrap_unirecipaddr *new_ip = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);
            memcpy(&new_ip->ip, &((ip_addr_t *) value)[i], sizeof(ip_addr_t));
            PyList_Append(list, (PyObject *) new_ip);
            Py_DECREF(new_ip);
         }
         return list;
    case UR_TYPE_A_MAC:
         for (i = 0; i < array_len; i++) {
            pytrap_unirecmacaddr *new_mac = (pytrap_unirecmacaddr *) pytrap_UnirecMACAddr.tp_alloc(&pytrap_UnirecMACAddr, 0);
            memcpy(&new_mac->mac, &((mac_addr_t *) value)[i], sizeof(mac_addr_t));
            PyList_Append(list, (PyObject *) new_mac);
            Py_DECREF(new_mac);
         }
         return list;
    case UR_TYPE_A_TIME:
         for (i = 0; i < array_len; i++) {
            pytrap_unirectime *new_time = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
            new_time->timestamp = ((ur_time_t *) value)[i];
            PyList_Append(list, (PyObject *) new_time);
            Py_DECREF(new_time);
         }
         return list;
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

    if (!PyUnicode_Check(field_name))
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

    if (ur_is_present(self->urtmplt, field_id) == 0) {
        PyErr_SetString(TrapError, "Field is not in the UniRec template");
        return NULL;
    }

    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);
    int i;

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
    case UR_TYPE_MAC:
        if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecMACAddr)) {
            pytrap_unirecmacaddr *src = ((pytrap_unirecmacaddr *) valueObj);
            mac_addr_t *dest = (mac_addr_t *) value;
            memcpy(dest, &src->mac, sizeof(mac_addr_t));
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
#if PY_MAJOR_VERSION >= 3
            const char *str = NULL;
            if (!PyUnicode_Check(valueObj)) {
                PyErr_SetString(PyExc_TypeError, "String object expected.");
                return NULL;
            }
            str = PyUnicode_AsUTF8AndSize(valueObj, &size);
#else
            char *str = NULL;
            if (!PyString_Check(valueObj)) {
                PyErr_SetString(PyExc_TypeError, "String object expected.");
                return NULL;
            }
            if (PyString_AsStringAndSize(valueObj, &str, &size) == -1) {
                return NULL;
            }
#endif
            if (str != NULL) {
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
                ur_set_var(self->urtmplt, data, field_id, str, size);
            }
        }
        break;
    case UR_TYPE_A_UINT8:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint8_t *) value)[0] = (uint8_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint8_t *) value)[0] = (uint8_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((uint8_t *) value)[i] = (uint8_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_INT8:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((int8_t *) value)[0] = (int8_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint8_t *) value)[0] = (uint8_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((int8_t *) value)[i] = (int8_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_UINT16:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint16_t *) value)[0] = (uint16_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint16_t *) value)[0] = (uint16_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((uint16_t *) value)[i] = (uint16_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_INT16:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((int16_t *) value)[0] = (int16_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint16_t *) value)[0] = (uint16_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((int16_t *) value)[i] = (int16_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_UINT32:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint32_t *) value)[0] = (uint32_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint32_t *) value)[0] = (uint32_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((uint32_t *) value)[i] = (uint32_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_INT32:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((int32_t *) value)[0] = (int32_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint32_t *) value)[0] = (uint32_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((int32_t *) value)[i] = (int32_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_UINT64:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint64_t *) value)[0] = (uint64_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint64_t *) value)[0] = (uint64_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((uint64_t *) value)[i] = (uint64_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_INT64:
        {
            if (PyLong_Check(valueObj)) {
                longval = PyLong_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((int64_t *) value)[0] = (int64_t) longval;
                }
#if PY_MAJOR_VERSION < 3
            } else if (PyInt_Check(valueObj)) {
                longval = PyInt_AsLong(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((uint64_t *) value)[0] = (uint64_t) longval;
                }
#endif
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  longval = PyLong_AsLong(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((int64_t *) value)[i] = (int64_t) longval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of long or list of longs.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_FLOAT:
        {
            if (PyFloat_Check(valueObj)) {
                floatval = PyFloat_AsDouble(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((float *) value)[0] = (float) floatval;
                }
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  floatval = PyFloat_AsDouble(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((float *) value)[i] = (float) floatval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of float or list of floats.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_DOUBLE:
        {
            if (PyFloat_Check(valueObj)) {
                floatval = PyFloat_AsDouble(valueObj);
                if (PyErr_Occurred() == NULL) {
                   ur_array_allocate(self->urtmplt, data, field_id, 1);
                   ((double *) value)[0] = (double) floatval;
                }
            } else if (PyList_Check(valueObj)) {
               ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
               for (i = 0; i < PyList_Size(valueObj); i++) {
                  floatval = PyFloat_AsDouble(PyList_GetItem(valueObj, i));
                  if (PyErr_Occurred() == NULL) {
                     ((double *) value)[i] = (double) floatval;
                  } else {
                     break;
                  }
               }
            } else {
               PyErr_SetString(PyExc_TypeError, "Argument data must be of float or list of floats.");
               return NULL;
            }
        }
        break;
    case UR_TYPE_A_IP:
        {
           if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecIPAddr)) {
              pytrap_unirecipaddr *src = ((pytrap_unirecipaddr *) valueObj);
              ip_addr_t *dest = (ip_addr_t *) value;
              dest->ui64[0] = src->ip.ui64[0];
              dest->ui64[1] = src->ip.ui64[1];
           } else if (PyList_Check(valueObj)) {
              ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
              for (i = 0; i < PyList_Size(valueObj); i++) {
                 PyObject *tmp = PyList_GetItem(valueObj, i);
                 if (PyObject_IsInstance(tmp, (PyObject *) &pytrap_UnirecIPAddr)) {
                    pytrap_unirecipaddr *src = (pytrap_unirecipaddr *) tmp;
                    ip_addr_t *dest = &((ip_addr_t *) value)[i];
                    dest->ui64[0] = src->ip.ui64[0];
                    dest->ui64[1] = src->ip.ui64[1];
                 } else {
                    PyErr_SetString(PyExc_TypeError, "Argument data must be of UnirecIPAddr or list of UnirecIPAddr.");
                    return NULL;
                 }
              }
           } else {
              PyErr_SetString(PyExc_TypeError, "Argument data must be of UnirecIPAddr or list of UnirecIPAddr.");
              return NULL;
           }
        }
        break;
    case UR_TYPE_A_MAC:
        {
           if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecMACAddr)) {
              pytrap_unirecmacaddr *src = ((pytrap_unirecmacaddr *) valueObj);
              mac_addr_t *dest = (mac_addr_t *) value;
              memcpy(dest, &src->mac, sizeof(mac_addr_t));
           } else if (PyList_Check(valueObj)) {
              ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
              for (i = 0; i < PyList_Size(valueObj); i++) {
                 PyObject *tmp = PyList_GetItem(valueObj, i);
                 if (PyObject_IsInstance(tmp, (PyObject *) &pytrap_UnirecMACAddr)) {
                    pytrap_unirecmacaddr *src = (pytrap_unirecmacaddr *) tmp;
                    mac_addr_t *dest = &((mac_addr_t *) value)[i];
                    memcpy(dest, &src->mac, sizeof(mac_addr_t));
                 } else {
                    PyErr_SetString(PyExc_TypeError, "Argument data must be of UnirecMACAddr or list of UnirecMACAddr.");
                    return NULL;
                 }
              }
           } else {
              PyErr_SetString(PyExc_TypeError, "Argument data must be of UnirecMACAddr or list of UnirecMACAddr.");
              return NULL;
           }
        }
        break;
    case UR_TYPE_A_TIME:
        {
           if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecTime)) {
              pytrap_unirectime *src = ((pytrap_unirectime *) valueObj);
              *((ur_time_t *) value) = src->timestamp;
           } else if (PyList_Check(valueObj)) {
              ur_array_allocate(self->urtmplt, data, field_id, PyList_Size(valueObj));
              for (i = 0; i < PyList_Size(valueObj); i++) {
                 PyObject *tmp = PyList_GetItem(valueObj, i);
                 if (PyObject_IsInstance(tmp, (PyObject *) &pytrap_UnirecTime)) {
                    pytrap_unirectime *src = (pytrap_unirectime *) tmp;
                    ((ur_time_t *) value)[i] = src->timestamp;
                 } else {
                    PyErr_SetString(PyExc_TypeError, "Argument data must be of UnirecTime or list of UnirecTime.");
                    return NULL;
                 }
              }
           } else {
              PyErr_SetString(PyExc_TypeError, "Argument data must be of UnirecTime or list of UnirecTime.");
              return NULL;
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

    return PyByteArray_FromStringAndSize(self->data, ur_rec_size(self->urtmplt, self->data));
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

    if (!PyUnicode_Check(field_name)) {
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
UnirecTemplate_getFieldsDict_local(pytrap_unirectemplate *self, char byId)
{
    PyObject *key, *num;
    int i;
    int result;
    PyObject *d = PyDict_New();
    if (d != NULL) {
        for (i = 0; i < self->urtmplt->count; i++) {
            key = PyUnicode_FromString(ur_get_name(self->urtmplt->ids[i]));
            num = PyLong_FromLong(self->urtmplt->ids[i]);
            if (byId) {
                result = PyDict_SetItem(d, num, key);
            } else {
                result = PyDict_SetItem(d, key, num);
            }
            Py_DECREF(key);
            Py_DECREF(num);
            if (result == -1) {
               fprintf(stderr, "failed to set item dict.\n");
               Py_RETURN_NONE;
            }
        }
        return d;
    }
    Py_XDECREF(d);
    Py_RETURN_NONE;
}

PyObject *
UnirecTemplate_getFieldsDict(pytrap_unirectemplate *self, PyObject *args)
{
    int idkey = 0;

    if (!PyArg_ParseTuple(args, "|p", &idkey)) {
        return NULL;
    }
    return UnirecTemplate_getFieldsDict_local(self, idkey);

}

PyObject *
UnirecTemplate_setFromDict(pytrap_unirectemplate *self, PyObject *dict, int skip_errors)
{
    // Create message if missing
    if (self->data_obj == NULL) {
        PyObject *res = UnirecTemplate_createMessage(self, (uint32_t) 1000);
        if (res == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Could not allocate new message memory.");
            return NULL;
        }
    }

    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_TypeError, "setFromDict() expects dict() argument.");
        return NULL;
    }

    if (!PyDict_Size((PyObject *) dict)) {
        Py_RETURN_NONE;
    }

    ur_field_id_t id = UR_ITER_BEGIN;
    while ((id = ur_iter_fields(self->urtmplt, id)) != UR_ITER_END) {
        PyObject *idkey = Py_BuildValue("i", id);
        if (!idkey) {
            //printf("failed idkey\n");
            return NULL;
        }
        PyObject *keyfield = PyDict_GetItem((PyObject *) self->fields_dict, idkey);
        Py_DECREF(idkey);

        if (!keyfield) {
            continue;
        }
        PyObject *v = PyDict_GetItem((PyObject *) dict, keyfield);

        if (!v) {
            if (skip_errors != 0) {
                continue;
            } else {
                PyErr_Format(PyExc_IndexError, "Key %s was not found in the dictionary.", ur_get_name(id));
                return NULL;
            }
        } else {
            if (PyUnicode_Check(v)) {
                const char *cstr = PyUnicode_AsUTF8(v);
                int res = ur_set_from_string(self->urtmplt, self->data, id, cstr);
                if (res != 0) {
                    PyErr_SetString(TrapError, "Could not set field.");
                    Py_DECREF(idkey);
                    return NULL;
                }
            } else {
                if (UnirecTemplate_set_local(self, self->data, id, v) == NULL) {
                    Py_DECREF(idkey);
                    return NULL;
                }
            }
        }
    }
    Py_RETURN_NONE;
}

PyObject *
UnirecTemplate_setFromDict_py(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds)
{
    // Create message if missing
    PyDictObject *dict;
    int skip_errors = 1;

    static char *kwlist[] = {"data", "skip_errors", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|p", kwlist, &PyDict_Type, &dict, &skip_errors)) {
        return NULL;
    }

    return UnirecTemplate_setFromDict(self, (PyObject *) dict, skip_errors);
}

PyObject *
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

PyObject *
UnirecTemplate_createMessage(pytrap_unirectemplate *self, uint32_t dynamic_size)
{
    char *data;
    uint32_t data_size = dynamic_size;

    if (self->data != NULL) {
        /* decrease refCount of the previously stored data */
        Py_DECREF(self->data_obj);
        self->data = NULL;
        self->data_obj = NULL;
    }

    data_size += ur_rec_fixlen_size(self->urtmplt);
    if (data_size > UR_MAX_SIZE) {
        PyErr_Format(TrapError, "Size of message is %d B, which is more than maximum %d bytes.",
                     data_size, UR_MAX_SIZE);
        return NULL;
    }
    data = ur_create_record(self->urtmplt, (uint16_t) data_size);
    PyObject *res = PyByteArray_FromStringAndSize(data, data_size);

    self->data_obj = res;
    self->data_size = PyByteArray_Size(res);
    self->data = PyByteArray_AsString(res);
    /* Increment refCount for the original object, so it's not free'd */
    Py_INCREF(self->data_obj);
    free(data);

    return res;
}

static PyObject *
UnirecTemplate_createMessage_py(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds)
{
    uint32_t data_size = 0;

    static char *kwlist[] = {"dyn_size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|I", kwlist, &data_size)) {
        return NULL;
    }
    return UnirecTemplate_createMessage(self, data_size);
}


pytrap_unirectemplate *
UnirecTemplate_init(pytrap_unirectemplate *self)
{
    self->data = NULL;
    self->data_size = 0;
    if (self->data_obj != NULL) {
        Py_DECREF(self->data_obj);
        self->data_obj = NULL;
    }
    if (self->urdict != NULL) {
        Py_DECREF(self->urdict);
    }
    if (self->fields_dict != NULL) {
        Py_DECREF(self->fields_dict);
    }
    self->urdict = (PyDictObject *) UnirecTemplate_getFieldsDict_local(self, 0);
    self->fields_dict = (PyDictObject *) UnirecTemplate_getFieldsDict_local(self, 1);

    self->iter_index = 0;
    self->field_count = PyDict_Size((PyObject *) self->urdict);
    return self;
}

static PyObject *
UnirecTemplate_copy(pytrap_unirectemplate *self)
{
    pytrap_unirectemplate *n;
    n = (pytrap_unirectemplate *) pytrap_UnirecTemplate.tp_alloc(&pytrap_UnirecTemplate, 0);

    char *errmsg;
    char *spec = ur_template_string_delimiter(self->urtmplt, ',');
    if (spec == NULL) {
        PyErr_SetString(TrapError, "Creation of UniRec template failed. Could not get list of fields.");
        return NULL;
    }
    char *field_names = ur_ifc_data_fmt_to_field_names(spec);
    free(spec);
    if (field_names == NULL) {
        PyErr_SetString(TrapError, "Creation of UniRec template failed. Could not get list of fields.");
        return NULL;
    }
    n->urtmplt = ur_create_template(field_names, &errmsg);

    if (n->urtmplt == NULL) {
        PyErr_Format(TrapError, "Creation of UniRec template failed. %s (%s)", errmsg, field_names);
        Py_DECREF(n);
        free(field_names);
        return NULL;
    }
    free(field_names);

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
        Py_XDECREF(i);
        Py_XDECREF(val);
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

PyObject *
UnirecTemplate_getDict(pytrap_unirectemplate *self)
{
    if (self->data == NULL) {
        PyErr_SetString(TrapError, "Data was not set yet.");
        return NULL;
    }
    PyObject *d = PyDict_New();
    PyObject *key;
    PyObject *val;

    ur_field_id_t id = UR_ITER_BEGIN;
    while ((id = ur_iter_fields(self->urtmplt, id)) != UR_ITER_END) {
        key = PyUnicode_FromString(ur_get_name(id));
        val = UnirecTemplate_get_local(self, self->data, id);
        if (val) {
            PyDict_SetItem(d, key, val);
            Py_DECREF(val);
        } else {
            PyErr_Print();
            PyErr_Format(TrapError, "Could not encode value of %s field.", ur_get_name(id));
            Py_DECREF(key);
            Py_DECREF(d);
            return NULL;
        }
        Py_DECREF(key);
    }
    return d;
}

static PyObject *
UnirecTemplate_getFieldType(pytrap_unirectemplate *self, PyObject *args)
{
    PyObject *name, *result = NULL;

    if (!PyArg_ParseTuple(args, "O!", &PyUnicode_Type, &name)) {
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
        result = (PyObject *) &PyLong_Type;
        break;
    case UR_TYPE_FLOAT:
    case UR_TYPE_DOUBLE:
        result = (PyObject *) &PyFloat_Type;
        break;
    case UR_TYPE_IP:
        result = (PyObject *) &pytrap_UnirecIPAddr;
        break;
    case UR_TYPE_MAC:
        result = (PyObject *) &pytrap_UnirecMACAddr;
        break;
    case UR_TYPE_TIME:
        result = (PyObject *) &pytrap_UnirecTime;
        break;
    case UR_TYPE_STRING:
        result = (PyObject *) &PyUnicode_Type;
        break;
    case UR_TYPE_BYTES:
        result = (PyObject *) &PyByteArray_Type;
        break;
    case UR_TYPE_A_UINT8:
    case UR_TYPE_A_INT8:
    case UR_TYPE_A_UINT16:
    case UR_TYPE_A_INT16:
    case UR_TYPE_A_UINT32:
    case UR_TYPE_A_INT32:
    case UR_TYPE_A_UINT64:
    case UR_TYPE_A_INT64:
    case UR_TYPE_A_FLOAT:
    case UR_TYPE_A_DOUBLE:
    case UR_TYPE_A_IP:
    case UR_TYPE_A_MAC:
    case UR_TYPE_A_TIME:
      result = (PyObject *) &PyList_Type;
      break;
    default:
        PyErr_SetString(PyExc_NotImplementedError, "Unknown UniRec field type.");
        return NULL;
    } // case (field type)

    if (result != NULL) {
        Py_INCREF(result);
        return result;
    }
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
            PyErr_SetString(PyExc_TypeError, "Data was not set nor explicitly passed as argument.");
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

        {"getDict", (PyCFunction) UnirecTemplate_getDict, METH_NOARGS,
            "Get UniRec record as a dictionary.\n\n"
            "Returns:\n"
            "    dict(str, object): Dictionary of field names and field values.\n"
        },

        {"setFromDict", (PyCFunction) UnirecTemplate_setFromDict_py, METH_VARARGS | METH_KEYWORDS,
            "Set UniRec record from a give dictionary. UniRec template is used to iterate over fields, so the fields from dict not in UniRec template are skipped.\n\n"
            "Example:\n"
            "    >>> rec = pytrap.UnirecTemplate(\"ipaddr SRC_IP,ipaddr DST_IP,uint16 SRC_PORT,uint16 DST_PORT\")\n"
            "    >>> rec.setFromDict({\"SRC_IP\": \"10.0.0.1\", \"DST_IP\": \"10.0.0.2\", \"SRC_PORT\": 12111, \"DST_PORT\": 80})\n"
            "    >>> print(rec)\n"
            "    (ipaddr DST_IP,ipaddr SRC_IP,uint16 DST_PORT,uint16 SRC_PORT)\n"
            "    >>> rec.strRecord()\n"
            "    SRC_IP = UnirecIPAddr('10.0.0.1'), DST_IP = UnirecIPAddr('10.0.0.2'), SRC_PORT = 12111, DST_PORT = 80\n\n"
            "Args:\n"
            "    data (dict()): dictionary, UniRec fields are set to the values of matching keys.\n"
            "    skip_errors (Optional[bool]): Skip UniRec fields that are missing in dict if True, otherwise, raise IndexError when key is missing in dict.\n\n"
            "Raises:\n"
            "    IndexError: If skip_errors is False, raise exception on a missing key.\n"
            "\n"
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

        {"getFieldsDict", (PyCFunction) UnirecTemplate_getFieldsDict, METH_VARARGS,
            "Get set of fields of the template.\n\n"
            "Args:\n"
            "    byId (Optional[bool]): True - use field_id as key, False (default) - use name as key\n"
            "Returns:\n"
            "    Dict(str,int): Dictionary of field_id with field name as a key.\n"
        },

        {"createMessage", (PyCFunction) UnirecTemplate_createMessage_py, METH_VARARGS | METH_KEYWORDS,
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
    char *errmsg;

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
        char *field_names = ur_ifc_data_fmt_to_field_names(spec);
        if (field_names == NULL) {
            PyErr_SetString(TrapError, "Creation of UniRec template failed. Could not get list of fields.");
            return NULL;
        }
        self->urtmplt = ur_create_template(field_names, &errmsg);
        free(field_names);
        if (self->urtmplt == NULL) {
            PyErr_Format(TrapError, "Creation of UniRec template failed. %s", errmsg);
            Py_DECREF(self);
            return NULL;
        }

        self = UnirecTemplate_init(self);
    }

    return (PyObject *) self;
}

static void UnirecTemplate_dealloc(pytrap_unirectemplate *self)
{
    Py_XDECREF(self->urdict);
    Py_XDECREF(self->fields_dict);
    if (self->urtmplt) {
        ur_free_template(self->urtmplt);
    }
    Py_XDECREF(self->data_obj);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *
UnirecTemplate_str(pytrap_unirectemplate *self)
{
    char *s = ur_template_string_delimiter(self->urtmplt, ',');
    PyObject *result;
    result = PyUnicode_FromFormat("(%s)", s);
    free(s);
    return result;
}

PyObject *
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
    PyObject *result;

    if (self->iter_index < self->field_count) {
        name = PyUnicode_FromString(ur_get_name(self->urtmplt->ids[self->iter_index]));
        value = UnirecTemplate_get_local(self, self->data, self->urtmplt->ids[self->iter_index]);
        self->iter_index++;
        result = Py_BuildValue("(OO)", name, value);
        Py_DECREF(name);
        Py_DECREF(value);
        return result;
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

PyTypeObject pytrap_UnirecTemplate = {
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
    "UnirecTemplate(spec)\n"
    "    Class for UniRec template storage and base data access.\n\n"
    "    Example:\n"
    "        UnirecTemplate(\"ipaddr SRC_IP,uint16 SRC_PORT,time START\")\n"
    "        creates a template with tree fields.\n\n"
    "    Args:\n"
    "        spec (str): UniRec template specifier - list of field types and names\n", /* tp_doc */
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

    /* Add MACAddr */
    if (PyType_Ready(&pytrap_UnirecMACAddr) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecMACAddr);
    PyModule_AddObject(m, "UnirecMACAddr", (PyObject *) &pytrap_UnirecMACAddr);

    /* Add MACAddrRange */
    if (PyType_Ready(&pytrap_UnirecMACAddrRange) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecMACAddrRange);
    PyModule_AddObject(m, "UnirecMACAddrRange", (PyObject *) &pytrap_UnirecMACAddrRange);

    /* Add Template */
    if (PyType_Ready(&pytrap_UnirecTemplate) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecTemplate);
    PyModule_AddObject(m, "UnirecTemplate", (PyObject *) &pytrap_UnirecTemplate);

    PyDateTime_IMPORT;

    return EXIT_SUCCESS;
}


