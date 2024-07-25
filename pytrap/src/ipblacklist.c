#include <Python.h>
#include <structmember.h>
#include <inttypes.h>

#include <unirec/unirec.h>
#include "unirecipaddr.h"
#include "ipblacklist.h"
#include "pytrapexceptions.h"

/*********************/
/*    UnirecIPList   */
/*********************/

static ipps_network_list_t *
load_networks(PyDictObject *d)
{
    uint32_t i = 0;
    uint32_t struct_count = 50; // Starting v4_count of structs to alloc

    // ************* LOAD NETWORKS ********************** //

    // Alloc memory for networks structs, if malloc fails return NULL
    ipps_network_t *networks = malloc(struct_count * sizeof(ipps_network_t));
    if (networks == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed allocating memory for IP prefix search structures.");
        return NULL;
    }

    // Alloc memory for networks list, if malloc fails return NULL
    ipps_network_list_t *networks_list = malloc(sizeof(ipps_network_list_t));
    if (networks_list == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed allocating memory for IP prefix search structures.");
        return NULL;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next((PyObject *) d, &pos, &key, NULL)) {
        ipps_network_t *network = &networks[i];
        if (PyObject_IsInstance(key, (PyObject *) &pytrap_UnirecIPAddrRange)) {
            pytrap_unirecipaddrrange *r = (pytrap_unirecipaddrrange *) key;
            network->mask = r->mask;
            memcpy(&network->addr, &r->start->ip, sizeof(ip_addr_t));
            // If limit is reached alloc new memory
            if (i >= struct_count) {
                struct_count += 10;
                // If realloc fails return NULL
                if ((networks = realloc(networks, struct_count * sizeof(ipps_network_t))) == NULL) {
                    PyErr_SetString(PyExc_MemoryError, "Failed in reallocating network structure.");
                    return NULL;
                }
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "Unsupported type.");
            return NULL;
        }

        // store data - address of a pointer to value
        network->data_len = sizeof(PyObject **);
        network->data = malloc(network->data_len);
        if (network->data == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed allocating memory for user data.");
            return NULL;
        }
        value = PyDict_GetItem((PyObject *) d, key);
        Py_INCREF(value);
        PyObject **pv = &value;
        *((PyObject **) network->data) = *pv;

        i++;
    }

    networks_list->net_count = i;
    networks_list->networks = networks;

    return networks_list;
}

static PyObject *
UnirecIPList_find(PyObject *o, PyObject *args)
{
    int search_result;
    PyObject **data = NULL;
    pytrap_unirecipaddr *ip;
    pytrap_unireciplist *self = (pytrap_unireciplist *) o;

    if (!PyArg_ParseTuple(args, "O!", &pytrap_UnirecIPAddr, &ip)) {
        return NULL;
    }

    search_result = ipps_search(&ip->ip, self->ipps_ctx, (void ***) &data);
    if (search_result > 0) {

        // The stored value is an address of a pointer, we set *res that
        // represents the target PyObject *
        // The value was stored in load_networks()
        PyObject **o = (PyObject **) data[0];
        PyObject *res = (*o);

        // For debug:
        //    PyObject *v_str = PyObject_CallMethod(res, "__str__", "");
        //    const char *cstr = PyUnicode_AsUTF8(v_str);
        //    fprintf(stderr, "loaded: %s\n", cstr);
        //    Py_DECREF(v_str);

        Py_INCREF(res);
        return res;
    } else {
        Py_RETURN_NONE;
    }
}

static int
UnirecIPList_contains(PyObject *o, PyObject *value)
{
    pytrap_unireciplist *self = (pytrap_unireciplist *) o;

    if (!PyObject_IsInstance(value, (PyObject *) &pytrap_UnirecIPAddr)) {
        PyErr_SetString(PyExc_TypeError, "UnirecIPList.__contains__() expects UnirecIPAddr only.");
        return -1;
    }
    pytrap_unirecipaddr *ip = (pytrap_unirecipaddr *) value;

    // we don't need output here
    void **data;

    int search_result = ipps_search(&ip->ip, self->ipps_ctx, (void ***) &data);

    return (search_result > 0);
}

static PyMethodDef pytrap_unireciplist_methods[] = {
    {"find", (PyCFunction) UnirecIPList_find, METH_VARARGS,
        "Find an IP address in the list and get stored value.\n\n"
        "Returns:\n"
        "    object: None if the value was not found.\n"
        },

    {NULL, NULL, 0, NULL}
};

static PySequenceMethods UnirecIPList_seqmethods = {
    0, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    (objobjproc) UnirecIPList_contains, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
};

static void
destroy_networks(ipps_network_list_t *network_list) {
    uint32_t index;
    for (index = 0; index < network_list->net_count; index++) {
        free(network_list->networks[index].data);
    }

    free(network_list->networks);
    free(network_list);
}

int
UnirecIPList_init(pytrap_unireciplist *s, PyObject *args, PyObject *kwds)
{
    if (s == NULL) {
        return -1;
    }
    PyDictObject *dict = NULL;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict)) {
        return -1;
    }

    ipps_network_list_t *network_list = load_networks(dict);
    if (network_list == NULL) {
        // Exception was set by load_networks
        return -1;
    }

    s->ipps_ctx = ipps_init(network_list);

    if (s->ipps_ctx == NULL) {
        PyErr_SetString(PyExc_TypeError, "Init of ip_prefix_search module failed.");
        return -1;
    }
    destroy_networks(network_list);

    return 0;
}

static PyObject *
UnirecIPList_repr(pytrap_unireciplist *self)
{
    //char str[INET6_ADDRSTRLEN];
    //ip_to_str(&self->ip, str);
    return PyUnicode_FromFormat("UnirecIPList('%s')", "  ");
}

static PyObject *
UnirecIPList_str(pytrap_unireciplist *self)
{
    /* Print all ip intervals and data */
    uint32_t index = 0;
    char ip_string_start[INET6_ADDRSTRLEN];
    char ip_string_end[INET6_ADDRSTRLEN];

    PyObject *list = PyList_New(0);

    PyObject *str = PyUnicode_FromFormat("IPv4:\n%16s\t%16s\t%s\n", "Low IP", "High IP", "Data");
    PyList_Append(list, str);
    Py_DECREF(str);

    /* IPv4 */
    if (self->ipps_ctx->v4_count > 0) {
        for (index = 0; index < self->ipps_ctx->v4_count; ++index) {
            ip_to_str(&self->ipps_ctx->v4_prefix_intervals[index].low_ip, ip_string_start);
            ip_to_str(&self->ipps_ctx->v4_prefix_intervals[index].high_ip, ip_string_end);
            str = PyUnicode_FromFormat("%16s\t%15s\t", ip_string_start, ip_string_end);
            PyList_Append(list, str);
            Py_DECREF(str);
            if (self->ipps_ctx->v4_prefix_intervals[index].data_array) {
                PyObject *v = *((PyObject **) self->ipps_ctx->v4_prefix_intervals[index].data_array[0]);
                PyObject *v_str = PyObject_CallMethod(v, "__str__", "");
                PyList_Append(list, v_str);
                Py_DECREF(v_str);
            }
            str = PyUnicode_FromString("\n");
            PyList_Append(list, str);
            Py_DECREF(str);
        }
    }

    str = PyUnicode_FromFormat("IPv6:\n%46s\t%46s\t\t%s\n", "Low IP", "High IP", "Data");
    PyList_Append(list, str);
    Py_DECREF(str);
    /* IPv6 */
    if (self->ipps_ctx->v6_count > 0) {
        for (index = 0; index < self->ipps_ctx->v6_count; ++index) {
            ip_to_str(&self->ipps_ctx->v6_prefix_intervals[index].low_ip, &ip_string_start[0]);
            ip_to_str(&self->ipps_ctx->v6_prefix_intervals[index].high_ip, &ip_string_end[0]);
            str = PyUnicode_FromFormat("\t%46s\t%46s\t", ip_string_start, ip_string_end);
            PyList_Append(list, str);
            Py_DECREF(str);
            if (self->ipps_ctx->v6_prefix_intervals[index].data_array) {
                PyObject *v = *((PyObject **) self->ipps_ctx->v6_prefix_intervals[index].data_array[0]);
                PyObject *v_str = PyObject_CallMethod(v, "__str__", "");
                PyList_Append(list, v_str);
                Py_DECREF(v_str);
            }
        }
    }
    str = PyUnicode_FromString("\n");
    PyList_Append(list, str);
    Py_DECREF(str);
    PyObject *resultstr = PyUnicode_Join(PyUnicode_FromString(""), list);
    Py_DECREF(list);
    return resultstr;
}

static void UnirecIPList_dealloc(pytrap_unireciplist *self)
{
    uint32_t index;
    /* Remove IPv4 user data */
    for (index = 0; index < self->ipps_ctx->v4_count; ++index)
    {
        if (self->ipps_ctx->v4_prefix_intervals && self->ipps_ctx->v4_prefix_intervals[index].data_array) {
            PyObject **p = (PyObject **) self->ipps_ctx->v4_prefix_intervals[index].data_array[0];
            Py_XDECREF(*p);
        }
    }
    /* Remove IPv6 user data */
    for (index = 0; index < self->ipps_ctx->v6_count; ++index)
    {
        if (self->ipps_ctx->v6_prefix_intervals && self->ipps_ctx->v6_prefix_intervals[index].data_array) {
            PyObject **p = (PyObject **) self->ipps_ctx->v6_prefix_intervals[index].data_array[0];
            Py_XDECREF(*p);
        }
    }
    ipps_destroy(self->ipps_ctx);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
UnirecIPList_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;
    int res;

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecIPList) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecIPAddr)) {
        result = Py_NotImplemented;
        Py_INCREF(result);
    }

    res = UnirecIPList_contains(a, b);
    if (res == -1) {
        PyErr_SetString(PyExc_TypeError, "Error during searching.");
        return NULL;
    } else if (result > 0) {
        result = Py_True;
    } else {
        result = Py_False;
    }
    Py_INCREF(result);
    return result;
}

PyTypeObject pytrap_UnirecIPList = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecIPList",          /* tp_name */
    sizeof(pytrap_unireciplist),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) UnirecIPList_dealloc,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    (reprfunc) UnirecIPList_repr, /* tp_repr */
    0,                         /* tp_as_number */
    &UnirecIPList_seqmethods,  /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,
    0,                         /* tp_call */
    (reprfunc) UnirecIPList_str,                         /* tp_str */
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
    (richcmpfunc) UnirecIPList_compare, /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unireciplist_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) UnirecIPList_init,                         /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};

