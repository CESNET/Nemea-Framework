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

ipps_network_list_t *load_networks(PyDictObject *d)
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

    PyObject *sys = PyImport_ImportModule("sys");

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next((PyObject *) d, &pos, &key, &value)) {
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
        // store data
        PyObject *getsizeof = PyObject_CallMethod(sys, "getsizeof", "O", value);
        network->data_len = PyLong_AsLong(getsizeof);
        Py_DECREF(getsizeof);

        network->data = malloc(network->data_len * sizeof(char));
        if (network->data == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed allocating memory for user data.");
            return NULL;
        }
        memcpy(network->data, value, network->data_len);
        PyObject *p = network->data;
        /* potentially dangerous since we copy the object and reset its refcounter */
        p->ob_refcnt = 0;
        Py_INCREF(p);
        i++;
    }
    Py_DECREF(sys);

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

        PyObject *o = data[0];
    //    const char *cstr = PyUnicode_AsUTF8(o);
    //    fprintf(stderr, "loaded: %s\n", cstr);

        Py_INCREF(o);
        return o;
    } else {
        Py_RETURN_NONE;
    }
}

static int
UnirecIPList_contains(PyObject *o, PyObject *args)
{
    pytrap_unirecipaddr *ip;
    if (!PyArg_ParseTuple(args, "O!", &pytrap_UnirecIPAddr, &ip)) {
        return NULL;
    }

    /* found */
    return 1;
    /* not found */
    return 0;
}

static PyMethodDef pytrap_unireciplist_methods[] = {
    {"find", (PyCFunction) UnirecIPList_find, METH_VARARGS,
        "Find an IP address in the list and get stored value.\n\n"
        "Returns:\n"
        "    object: None if the value was not found.\n"
        },

    {NULL, NULL, 0, NULL}
};

static PyNumberMethods UnirecIPList_numbermethods = {
    .nb_bool = 0, //(inquiry) UnirecIPList_bool,
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

void destroy_networks(ipps_network_list_t *network_list) {
    int index;
    for (index = 0; index < network_list->net_count; index++) {
        PyObject *p = (PyObject *) network_list->networks[index].data;
        // we have just one temporary copy, which is duplicated into internal structures of ipps
        free(p);
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
    printf("----------------------------IPv4----------------------------\n");
    int index = 0;
    int j = 0;
    char ip_string[INET6_ADDRSTRLEN];

    printf("\t%-16s \t%-16s\t%s\n", "Low IP", "High IP", "Data");

    /* Check print IPv4 */
    for (index = 0; index < self->ipps_ctx->v4_count; ++index) {
        ip_to_str(&self->ipps_ctx->v4_prefix_intervals[index].low_ip, &ip_string[0]);
        printf("\t%-16s", ip_string);
        ip_to_str(&self->ipps_ctx->v4_prefix_intervals[index].high_ip, &ip_string[0]);
        printf("\t%-15s", ip_string);
        printf("\t");
        if (self->ipps_ctx->v4_prefix_intervals[index].data_array) {
            PyObject *v = (PyObject *) self->ipps_ctx->v4_prefix_intervals[index].data_array[0];
            PyObject *v_str = PyObject_CallMethod(v, "__str__", "");
            const char *cstr = PyUnicode_AsUTF8(v_str);
            printf("\t%s\n", cstr);
            Py_DECREF(v_str);
        } else {
            printf("\n");
        }
    }

    printf("------------------------------------------------------------\n");

    printf("\n-------------------------IPv6-------------------------------\n");
    printf("\t%-46s \t%-46s\t\t%s\n", "Low IP", "High IP", "Data");
    /* Check print IPv6 */
    for(index = 0; index < self->ipps_ctx->v6_count; ++index) {
        ip_to_str(&self->ipps_ctx->v6_prefix_intervals[index].low_ip, &ip_string[0]);
        printf("\t%-46s", ip_string);
        ip_to_str(&self->ipps_ctx->v6_prefix_intervals[index].high_ip, &ip_string[0]);
        printf("\t%-46s", ip_string);
        printf("\t");
        if (self->ipps_ctx->v6_prefix_intervals[index].data_array) {
            PyObject *v = (PyObject *) self->ipps_ctx->v6_prefix_intervals[index].data_array[0];
            PyObject *v_str = PyObject_CallMethod(v, "__str__", "");
            const char *cstr = PyUnicode_AsUTF8(v_str);
            printf("\t%s\n", cstr);
            Py_DECREF(v_str);
        } else {
            printf("\n");
        }
    }
    printf("------------------------------------------------------------\n\n");
    return PyUnicode_FromString("<UnirecIPList>");
}

long
UnirecIPList_hash(pytrap_unireciplist *o)
{
    /* TODO */
    return (long) 0;
}

static void UnirecIPList_dealloc(pytrap_unireciplist *self)
{
    int index;
    // TODO - memory leaks... but we cannot simply free values - Py_DECREF leads to error
    // free() is not possible due to returned values by the _find()...
    //
    //// Decrement refcounters of the stored python objects - user data
    ///* Check print IPv4 */
    //for (index = 0; index < self->ipps_ctx->v4_count; ++index)
    //{
    //    if (self->ipps_ctx->v4_prefix_intervals && self->ipps_ctx->v4_prefix_intervals[index].data_array) {
    //        PyObject *p = (PyObject *) self->ipps_ctx->v4_prefix_intervals[index].data_array[0];
    //        fprintf(stderr, "deletion of object with refcnt %d\n", p->ob_refcnt);
    //        //Py_XDECREF(p);
    //        free(self->ipps_ctx->v4_prefix_intervals[index].data_array);
    //    }
    //}

    ////fprintf(stderr, "Freeing IPv6 user data.");
    /////* Check print IPv6 */
    ////for(index = 0; index < self->ipps_ctx->v6_count; ++index)
    ////{
    ////    if (self->ipps_ctx->v6_prefix_intervals && self->ipps_ctx->v6_prefix_intervals[index].data_array) {
    ////        PyObject *p = (PyObject *) self->ipps_ctx->v6_prefix_intervals[index].data_array[0];
    ////        fprintf(stderr, "deletion of object with refcnt %d\n", p->ob_refcnt);
    ////        Py_XDECREF(p);
    ////    }
    ////}
    //fprintf(stderr, "Destroying the rest.");
    // Take from ipps_destroy(self->ipps_ctx), data - python objects must
    // remain in memory for the case there are some values still in use:
    free(self->ipps_ctx->v4_prefix_intervals);
    free(self->ipps_ctx->v6_prefix_intervals);
    free(self->ipps_ctx);
    Py_TYPE(self)->tp_free((PyObject *) self);
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
    &UnirecIPList_numbermethods, /* tp_as_number */
    &UnirecIPList_seqmethods,  /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecIPList_hash,
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
    0, /* tp_richcompare */ //(richcmpfunc) UnirecIPList_compare,
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

