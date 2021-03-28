#include <Python.h>
#include <structmember.h>
#include <inttypes.h>

#include <unirec/unirec.h>
#include "unirecmacaddr.h"
#include "pytrapexceptions.h"

/*********************/
/*   UnirecMACAddr   */
/*********************/

#define MAC_MASK 0x0000FFFFFFFFFFFFLL

static PyObject *
UnirecMACAddr_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecMACAddr) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecMACAddr)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirecmacaddr *ur_a = (pytrap_unirecmacaddr *) a;
    pytrap_unirecmacaddr *ur_b = (pytrap_unirecmacaddr *) b;

    int res = mac_cmp(&ur_a->mac, &ur_b->mac);

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

int
mac_is_null(mac_addr_t *mac)
{
   mac_addr_t null = { .bytes = {0, 0, 0, 0, 0, 0} };
   if (mac_cmp(mac, &null) == 0) {
      return 1;
   } else {
      return 0;
   }
}

// mac bytes to integer
uint64_t
mac_b2i(uint8_t bytes[6])
{
   return (uint64_t) ((uint64_t) ntohl(*((uint32_t *) bytes)) << 16 | ntohs(*(((uint16_t *) bytes) + 2)));
}

// mac integer to bytes
void
mac_i2b(uint64_t mac, uint8_t bytes[6])
{
   *(uint32_t *) bytes = htonl(mac >> 16);
   *(uint16_t *) (bytes + 4) = htons(mac & 0xFFFF);
}

// mac incrementation
uint64_t
mac_inc(uint64_t mac)
{
   return ++mac & MAC_MASK;
}

// mac decrementation
uint64_t
mac_dec(uint64_t mac)
{
   if (mac == 0) {
      return MAC_MASK;
   }
   return --mac;
}

static PyObject *
UnirecMACAddr_isNull(pytrap_unirecmacaddr *self)
{
    if (mac_is_null(&self->mac)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static int
UnirecMACAddr_bool(pytrap_unirecmacaddr *self)
{
    /* bool(mac) == (not isNull(mac)) */
    if (mac_is_null(&self->mac)) {
        return 0;
    } else {
        return 1;
    }
}

static PyObject *
UnirecMACAddr_inc(pytrap_unirecmacaddr *self)
{
    pytrap_unirecmacaddr *mac;
    mac = (pytrap_unirecmacaddr *) pytrap_UnirecMACAddr.tp_alloc(&pytrap_UnirecMACAddr, 0);

    uint64_t tmp = mac_b2i(self->mac.bytes);
    tmp = mac_inc(tmp);
    mac_i2b(tmp, mac->mac.bytes);

    Py_INCREF(mac);
    return (PyObject *) mac;
}

static PyObject *
UnirecMACAddr_dec(pytrap_unirecmacaddr *self)
{
    pytrap_unirecmacaddr *mac;
    mac = (pytrap_unirecmacaddr *) pytrap_UnirecMACAddr.tp_alloc(&pytrap_UnirecMACAddr, 0);

    uint64_t tmp = mac_b2i(self->mac.bytes);
    tmp = mac_dec(tmp);
    mac_i2b(tmp, mac->mac.bytes);

    Py_INCREF(mac);
    return (PyObject *) mac;
}

static int
UnirecMACAddr_contains(PyObject *o, PyObject *v)
{
    if (PyObject_IsInstance(v, (PyObject *) &pytrap_UnirecMACAddr)) {
        pytrap_unirecmacaddr *object = (pytrap_unirecmacaddr *) o;
        pytrap_unirecmacaddr *value = (pytrap_unirecmacaddr *) v;

        if (mac_cmp(&object->mac, &value->mac) == 0) {
            return 1;
        } else {
            return 0;
        }

    } else {
        PyErr_SetString(PyExc_TypeError, "UnirecMACAddr object expected.");
        return -1;
    }
}


static PyMethodDef pytrap_unirecmacaddr_methods[] = {
    {"isNull", (PyCFunction) UnirecMACAddr_isNull, METH_NOARGS,
        "Check if the MAC address is null, i.e. \"00:00:00:00:00:00\".\n\n"
        "Returns:\n"
        "    bool: True if the address is null.\n"
        },

    {"inc", (PyCFunction) UnirecMACAddr_inc, METH_NOARGS,
        "Increment MAC address.\n\n"
        "Returns:\n"
        "    UnirecMACAddr: New incremented MAC address.\n"
        },

    {"dec", (PyCFunction) UnirecMACAddr_dec, METH_NOARGS,
        "Decrement MAC address.\n\n"
        "Returns:\n"
        "    UnirecMACAddr: New decremented MAC address.\n"
        },
    {NULL, NULL, 0, NULL}
};

static PyNumberMethods UnirecMACAddr_numbermethods = {
#if PY_MAJOR_VERSION >= 3
    .nb_bool = (inquiry) UnirecMACAddr_bool, 
#else
    .nb_nonzero = (inquiry) UnirecMACAddr_bool,
#endif
};

static PySequenceMethods UnirecMACAddr_seqmethods = {
    0, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    (objobjproc) UnirecMACAddr_contains, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
};

int
UnirecMACAddr_init(pytrap_unirecmacaddr *s, PyObject *args, PyObject *kwds)
{
    char *mac_str;

    if (s != NULL) {
        if (!PyArg_ParseTuple(args, "s", &mac_str)) {
            return -1;
        }
        if (mac_from_str(mac_str, &s->mac) != 1) {
            PyErr_SetString(TrapError, "Could not parse given MAC address.");
            return -1;
        }
    } else {
        return -1;
    }
    return 0;

}

static PyObject *
UnirecMACAddr_repr(pytrap_unirecmacaddr *self)
{
    char str[MAC_STR_LEN];
    mac_to_str(&self->mac, str);
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("UnirecMACAddr('%s')", str);
#else
    return PyString_FromFormat("UnirecMACAddr('%s')", str);
#endif
}

static PyObject *
UnirecMACAddr_str(pytrap_unirecmacaddr *self)
{
    char str[MAC_STR_LEN];
    mac_to_str(&self->mac, str);
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(str);
#else
    return PyString_FromString(str);
#endif
}

long
UnirecMACAddr_hash(pytrap_unirecmacaddr *o)
{
    return (long) ((*((uint32_t *) &o->mac)) << 16 | *(((uint16_t *) &o->mac) + 2));
}

PyTypeObject pytrap_UnirecMACAddr = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecMACAddr",          /* tp_name */
    sizeof(pytrap_unirecmacaddr),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    (reprfunc) UnirecMACAddr_repr, /* tp_repr */
    &UnirecMACAddr_numbermethods, /* tp_as_number */
    &UnirecMACAddr_seqmethods,  /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecMACAddr_hash,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecMACAddr_str,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "UnirecMACAddr(mac)\n"
    "    Class for UniRec MAC Address storage and base data access.\n\n"
    "    Args:\n"
    "        mac (str): text represented MAC address (e.g. \"00:11:22:33:44:55\")\n", /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc) UnirecMACAddr_compare,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unirecmacaddr_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) UnirecMACAddr_init,                         /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};

/*************************/
/*    UnirecMACAddrRange  */
/*************************/

static void
UnirecMACAddrRange_dealloc(pytrap_unirecmacaddrrange *self)
{
    Py_XDECREF(self->start);
    Py_XDECREF(self->end);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
UnirecMACAddrRange_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirecmacaddrrange *self;

    self = (pytrap_unirecmacaddrrange *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->start = (pytrap_unirecmacaddr *) pytrap_UnirecMACAddr.tp_alloc(&pytrap_UnirecMACAddr, 0);

        if (self->start == NULL) {
            return NULL;
        }

        self->end = (pytrap_unirecmacaddr *) pytrap_UnirecMACAddr.tp_alloc(&pytrap_UnirecMACAddr, 0);

        if (self->end == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Allocation of end address failed.");
            Py_DECREF(self->start);
            return NULL;
        }
    }

    return (PyObject *)self;
}


static PyObject *
UnirecMACAddrRange_isIn(pytrap_unirecmacaddrrange *self, PyObject *args)
{
    pytrap_unirecmacaddr *macaddr = (pytrap_unirecmacaddr *) args;
    PyObject *result = Py_False;

    if (!PyObject_IsInstance(args, (PyObject *) &pytrap_UnirecMACAddr)) {
        result = Py_NotImplemented;
    }

    int cmp_result;
    cmp_result = mac_cmp(&self->start->mac, &macaddr->mac);

    if (cmp_result == 0) {
        /* mac address is in interval */
        result = PyLong_FromLong(0);
    } else if (cmp_result > 0) {
        /* mac address is lower then interval */
        result = PyLong_FromLong(-1);
    } else {
        cmp_result = mac_cmp(&self->end->mac, &macaddr->mac);
        if (cmp_result >= 0) {
            /* mac address is in interval */
            result = PyLong_FromLong(0);
        } else {
            /* mac address is greater then interval */
            result = PyLong_FromLong(1);
        }
    }

    return result;
}


static PyObject *
UnirecMACAddrRange_isOverlap(pytrap_unirecmacaddrrange *self, PyObject *args)
{
    /* compared ranges must by sorted by low mac */
    pytrap_unirecmacaddrrange *other;

    PyObject *tmp;
    long cmp_result;

    if (!PyArg_ParseTuple(args, "O", &other))
        return NULL;

    if (!PyObject_IsInstance((PyObject *) other, (PyObject *) &pytrap_UnirecMACAddrRange)) {
        return Py_NotImplemented;
    }

    tmp = UnirecMACAddrRange_isIn(self, (PyObject *) other->start);
    cmp_result = PyLong_AsLong(tmp);
    Py_DECREF(tmp);

    if (cmp_result == 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static int
UnirecMACAddrRange_contains(pytrap_unirecmacaddrrange *o, pytrap_unirecmacaddr *mac)
{
    PyObject * tmp = UnirecMACAddrRange_isIn(o, (PyObject *) mac);
    int cmp_result = PyLong_AsLong(tmp);
    Py_DECREF(tmp);
    return cmp_result == 0 ? 1 : 0;
}

#define FREE_CLEAR(p) free(p);

static int
UnirecMACAddrRange_init(pytrap_unirecmacaddrrange *self, PyObject *args, PyObject *kwds)
{
    PyObject *start = NULL, *end = NULL;

    static char *kwlist[] = {"start", "end", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &start, &end)) {
        goto exit_failure;
    }

    /* start argument */
    if (PyObject_IsInstance(start, (PyObject *) &pytrap_UnirecMACAddr)) {
        memcpy(&self->start->mac, &(((pytrap_unirecmacaddr *) start)->mac), sizeof(mac_addr_t));
    } else {
        goto exit_failure;
    }

    /* end argument */
    if (PyObject_IsInstance(end, (PyObject *) &pytrap_UnirecMACAddr)) {
        memcpy(&self->end->mac, &(((pytrap_unirecmacaddr *) end)->mac), sizeof(mac_addr_t));
    } else {
        goto exit_failure;
    }
    return 0;
exit_failure:
    return -1;
}

static PyObject *
UnirecMACAddrRange_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecMACAddrRange) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecMACAddrRange)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirecmacaddrrange *ur_a = (pytrap_unirecmacaddrrange *) a;
    pytrap_unirecmacaddrrange *ur_b = (pytrap_unirecmacaddrrange *) b;

    int res_S = mac_cmp(&ur_a->start->mac, &ur_b->start->mac);
    int res_E = mac_cmp(&ur_a->end->mac, &ur_b->end->mac);

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
UnirecMACAddrRange_repr(pytrap_unirecmacaddrrange *self)
{
    PyObject *mac1 = NULL, *mac2 = NULL, *res = NULL;

    mac1 = UnirecMACAddr_repr(self->start);
    mac2 = UnirecMACAddr_repr(self->end);
#if PY_MAJOR_VERSION >= 3
    res = PyUnicode_FromFormat("UnirecMACAddrRange(%S, %S)", mac1, mac2);
#else
    res = PyString_FromFormat("UnirecMACAddrRange(%s, %s)", PyString_AsString(mac1), PyString_AsString(mac2));
#endif
    Py_DECREF(mac1);
    Py_DECREF(mac2);
    return res;
}

static PyObject *
UnirecMACAddrRange_str(pytrap_unirecmacaddrrange *self)
{
    PyObject *mac1 = NULL, *mac2 = NULL, *res = NULL;
    mac1 = UnirecMACAddr_str(self->start);
    mac2 = UnirecMACAddr_str(self->end);
#if PY_MAJOR_VERSION >= 3
    res = PyUnicode_FromFormat("%S - %S", mac1, mac2);
#else
    res = PyString_FromFormat("%s - %s", PyString_AsString(mac1), PyString_AsString(mac2));
#endif
    Py_DECREF(mac1);
    Py_DECREF(mac2);
    return res;
}

long
UnirecMACAddrRange_hash(pytrap_unirecmacaddrrange *o)
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

static PyMemberDef UnirecMACAddrRange_members[] = {
    {"start", T_OBJECT_EX, offsetof(pytrap_unirecmacaddrrange, start), 0,
     "Low MAC address of range"},
    {"end", T_OBJECT_EX, offsetof(pytrap_unirecmacaddrrange, end), 0,
     "High MAC address of range"},
    {NULL}  /* Sentinel */
};


static PyMethodDef UnirecMACAddrRange_methods[] = {
    {"isIn", (PyCFunction) UnirecMACAddrRange_isIn, METH_O,
        "Check if the address is in MAC range.\n\n"
        "Args:\n"
        "    macaddr: UnirecMACAddr struct"
        "Returns:\n"
        "    long: -1 if macaddr < start of range, 0 if macaddr is in interval, 1 if macaddr > end of range .\n"
        },

    {"isOverlap", (PyCFunction) UnirecMACAddrRange_isOverlap, METH_VARARGS,
        "Check if 2 ranges sorted by start MAC overlap.\n\n"
        "Args:\n"
        "    other: UnirecMACAddrRange, second interval to compare"
        "Returns:\n"
        "    bool: True if ranges are overlaps.\n"
        },
    {NULL}  /* Sentinel */
};


static PySequenceMethods UnirecMACAddrRange_seqmethods = {
    0, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    (objobjproc) UnirecMACAddrRange_contains, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
};


PyTypeObject pytrap_UnirecMACAddrRange = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecMACAddrRange",       /* tp_name */
    sizeof(pytrap_unirecmacaddrrange), /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) UnirecMACAddrRange_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    (reprfunc) UnirecMACAddrRange_repr, /* tp_repr */
    0,                         /* tp_as_number */
    &UnirecMACAddrRange_seqmethods, /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecMACAddrRange_hash, /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecMACAddrRange_str, /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "UnirecMACAddrRange(start, end)\n"
    "    Class for UniRec MAC Address Range storage and base data access.\n\n"
    "    Args:\n"
    "        start (UnirecMACAddr): text represented MAC address or prefix using /mask notation\n"
    "        end (UnirecMACAddr): text represented MAC address of end of the range\n"
    "        (start must not contain /mask)\n", /* tp_doc */
    0, /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc) UnirecMACAddrRange_compare, /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    UnirecMACAddrRange_methods, /* tp_methods */
    UnirecMACAddrRange_members, /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)UnirecMACAddrRange_init,      /* tp_init */
    0,                         /* tp_alloc */
    UnirecMACAddrRange_new,     /* tp_new */
};


