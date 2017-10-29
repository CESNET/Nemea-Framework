#include <Python.h>
#include <structmember.h>
#include <inttypes.h>

#include <unirec/unirec.h>
#include "unirecipaddr.h"
#include "pytrapexceptions.h"

/*********************/
/*    UnirecIPAddr   */
/*********************/

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

static int
UnirecIPAddr_contains(PyObject *o, PyObject *v)
{
    if (PyObject_IsInstance(v, (PyObject *) &pytrap_UnirecIPAddr)) {
        pytrap_unirecipaddr *object = (pytrap_unirecipaddr *) o;
        pytrap_unirecipaddr *value = (pytrap_unirecipaddr *) v;

        if (ip_cmp(&object->ip, &value->ip) == 0) {
            return 1;
        } else {
            return 0;
        }

    } else {
        PyErr_SetString(PyExc_TypeError, "UnirecIPAddr object expected.");
        return -1;
    }
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

static PySequenceMethods UnirecIPAddr_seqmethods = {
    0, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    (objobjproc) UnirecIPAddr_contains, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
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

PyTypeObject pytrap_UnirecIPAddr = {
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
    &UnirecIPAddr_numbermethods, /* tp_as_number */
    &UnirecIPAddr_seqmethods,  /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecIPAddr_hash,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecIPAddr_str,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "UnirecIPAddr(ip)\n"
    "    Class for UniRec IP Address storage and base data access.\n\n"
    "    Args:\n"
    "        ip (str): text represented IPv4 or IPv6 address\n", /* tp_doc */
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

/*************************/
/*    UnirecIPAddrRange  */
/*************************/

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
            return NULL;
        }

        self->end = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);

        if (self->end == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Allocation of end address failed.");
            Py_DECREF(self->start);
            return NULL;
        }
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

/* TODO something like this can be used globally */
#if PY_MAJOR_VERSION >= 3
#  define CHECK_STR_CONV(pobj, cstr, csize) { \
    if (!PyUnicode_Check(pobj)) { \
        PyErr_SetString(PyExc_TypeError, "String or UnirecIPAddr object expected."); \
        return -1; \
    } else { \
        cstr = PyUnicode_AsUTF8AndSize(pobj, &csize); \
    } \
} while (0)
#else
#  define CHECK_STR_CONV(pobj, cstr, csize) { \
    if (!PyString_Check(pobj)) { \
        PyErr_SetString(PyExc_TypeError, "String or UnirecIPAddr object expected."); \
        return -1; \
    } \
    if (PyString_AsStringAndSize(pobj, &cstr, &csize) == -1) { \
        return -1; \
    } \
} while (0)
#endif

#define COPY_IPADD(str, dest, tmp) { \
    if (ip_from_str(str, &tmp) != 1) { \
        PyErr_SetString(TrapError, "Could not parse given IP address."); \
        goto exit_failure; \
    } \
    memcpy(&dest, &tmp, sizeof(ip_addr_t)); \
} while (0)

#define FREE_CLEAR(p) free(p);

static int
UnirecIPAddrRange_init(pytrap_unirecipaddrrange *self, PyObject *args, PyObject *kwds)
{
    PyObject *start = NULL, *end = NULL;
    char *argstr = NULL, *str = NULL, *netmask = NULL;
    Py_ssize_t size;
    ip_addr_t tmp_ip;
    unsigned char mask = 0;
    uint32_t net_mask_array[4];

    static char *kwlist[] = {"start", "end", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &start, &end)) {
        goto exit_failure;
    }

    /* start argument */
    if (PyObject_IsInstance(start, (PyObject *) &pytrap_UnirecIPAddr)) {
        if (!end) {
            PyErr_SetString(PyExc_TypeError, "Missing end argument.");
            goto exit_failure;
        }
        memcpy(&self->start->ip, &(((pytrap_unirecipaddr *) start)->ip), sizeof(ip_addr_t));
    } else {
        /* check for supported string type */
        CHECK_STR_CONV(start, argstr, size);
        str = strdup(argstr);
        /* supported string type */
        netmask = strchr(str, '/');
        if (netmask == NULL) {
            if (!end) {
                PyErr_SetString(PyExc_TypeError, "End argument is required when start is not in <ip>/<mask> format.");
                goto exit_failure;
            }
            /* set start IP and continue with end */
            COPY_IPADD(str, self->start->ip, tmp_ip);
            FREE_CLEAR(str);
        } else {
            if (end) {
                PyErr_SetString(PyExc_TypeError, "Start argument is in <ip>/<mask> format - end argument must be ommited.");
                goto exit_failure;
            }

            /* expected string in <ip>/<mask> format, e.g. 192.168.1.0/24 */
            *netmask = '\0';
            netmask++;
            if (sscanf(netmask, "%" SCNu8, &mask) != 1) {
                PyErr_Format(PyExc_TypeError, "Malformed netmask %s.", netmask);
                goto exit_failure;
            }

            /* replace '/' with '\0' in order to convert string IP into ip_addr_t */
            if (ip_from_str(str, &tmp_ip) != 1) {
                PyErr_SetString(TrapError, "Could not parse given IP address.");
                goto exit_failure;
            }

            if (ip_is4(&tmp_ip)) {
                net_mask_array[0] = 0xFFFFFFFF >> (mask > 31 ? 0 : 32 - mask);
                net_mask_array[0] = (bit_endian_swap((net_mask_array[0] & 0x000000FF)>>  0) <<  0) |
                    (bit_endian_swap((net_mask_array[0] & 0x0000FF00)>>  8) <<  8) |
                    (bit_endian_swap((net_mask_array[0] & 0x00FF0000)>> 16) << 16) |
                    (bit_endian_swap((net_mask_array[0] & 0xFF000000)>> 24) << 24);

                tmp_ip.ui32[2] = tmp_ip.ui32[2] & net_mask_array[0];
                memcpy(&self->start->ip, &tmp_ip, 16);

                tmp_ip.ui32[2] = tmp_ip.ui32[2] | (~net_mask_array[0]);
                memcpy(&self->end->ip, &tmp_ip, 16);
            } else {
                // Fill every word of IPv6 address
                net_mask_array[0] = 0xFFFFFFFF>>(mask > 31 ? 0 : 32 - mask);
                net_mask_array[1] = 0xFFFFFFFF>>(mask > 63 ? 0 : (mask > 32 ? 64 - mask: 32));
                net_mask_array[2] = 0xFFFFFFFF>>(mask > 95 ? 0 : (mask > 64 ? 96 - mask: 32));
                net_mask_array[3] = 0xFFFFFFFF>>(mask > 127 ? 0 :(mask > 96 ? 128 - mask : 32));

                int i;

                for (i = 0; i < 4; ++i) {
                    // Swap bits in every byte for compatibility with ip_addr_t stucture
                    net_mask_array[i] = (bit_endian_swap((net_mask_array[i] & 0x000000FF) >> 0) <<  0) |
                        (bit_endian_swap((net_mask_array[i] & 0x0000FF00)>>  8) <<  8) |
                        (bit_endian_swap((net_mask_array[i] & 0x00FF0000)>> 16) << 16) |
                        (bit_endian_swap((net_mask_array[i] & 0xFF000000)>> 24) << 24);

                    self->start->ip.ui32[i] = tmp_ip.ui32[i] & net_mask_array[i];
                    self->end->ip.ui32[i] = tmp_ip.ui32[i] | ( ~ net_mask_array[i]);
                }
            }
            free(str);
            /* we are done */
            goto exit_success;
        }
    }

    /* end argument */
    if (PyObject_IsInstance(end, (PyObject *) &pytrap_UnirecIPAddr)) {
        memcpy(&self->end->ip, &(((pytrap_unirecipaddr *) end)->ip), sizeof(ip_addr_t));
    } else {
        /* string is supported */
        CHECK_STR_CONV(end, str, size);
        COPY_IPADD(str, self->end->ip, tmp_ip);
    }
exit_success:
    return 0;
exit_failure:
    FREE_CLEAR(str);
    return -1;
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
    PyObject *ip1 = NULL, *ip2 = NULL, *res = NULL;

    ip1 = UnirecIPAddr_repr(self->start);
    ip2 = UnirecIPAddr_repr(self->end);
#if PY_MAJOR_VERSION >= 3
    res = PyUnicode_FromFormat("UnirecIPAddrRange(%S, %S)", ip1, ip2);
#else
    res = PyString_FromFormat("UnirecIPAddrRange(%s, %s)", PyString_AsString(ip1), PyString_AsString(ip2));
#endif
    Py_DECREF(ip1);
    Py_DECREF(ip2);
    return res;
}

static PyObject *
UnirecIPAddrRange_str(pytrap_unirecipaddrrange *self)
{
    PyObject *ip1 = NULL, *ip2 = NULL, *res = NULL;
    ip1 = UnirecIPAddr_str(self->start);
    ip2 = UnirecIPAddr_str(self->end);
#if PY_MAJOR_VERSION >= 3
    res = PyUnicode_FromFormat("%S - %S", ip1, ip2);
#else
    res = PyString_FromFormat("%s - %s", PyString_AsString(ip1), PyString_AsString(ip2));
#endif
    Py_DECREF(ip1);
    Py_DECREF(ip2);
    return res;
}

long
UnirecIPAddrRange_hash(pytrap_unirecipaddrrange *o)
{
    PyObject *tuple = PyTuple_New(2);
    /* increase references because Tuple steals them */
    Py_INCREF(o->start);
    Py_INCREF(o->end);

    PyTuple_SetItem(tuple, 0, (PyObject *) o->start);
    PyTuple_SetItem(tuple, 1, (PyObject *) o->end);
    long hash = PyObject_Hash(tuple);

    Py_DECREF(tuple);
    return hash;
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


PyTypeObject pytrap_UnirecIPAddrRange = {
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
    (hashfunc) UnirecIPAddrRange_hash, /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecIPAddrRange_str, /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "UnirecIPAddrRange(start, [end])\n"
    "    Class for UniRec IP Address Range storage and base data access.\n\n"
    "    Args:\n"
    "        start (str): text represented IPv4 address or prefix using /mask notation\n"
    "        end (Optional[str]): text represented IPv4 address of end of the range\n"
    "        (start must not contain /mask)\n", /* tp_doc */
    0, /* tp_traverse */
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

