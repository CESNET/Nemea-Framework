#include <Python.h>
#include <structmember.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static trap_module_info_t *module_info = NULL;
static ur_template_t *in_tmplt = NULL;

#define MODULE_BASIC_INFO(BASIC) \
    BASIC("Example module","example.",1,0)

#define MODULE_PARAMS(PARAM)

static PyObject *TrapError;

static PyObject *TimeoutError;

static int
local_trap_init(int argc, char **argv, trap_module_info_t *module_info, int ifcin, int ifcout)
{
    INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
    module_info->num_ifc_in = ifcin;
    module_info->num_ifc_out = ifcout;

    TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);
    return 0;
}

static PyObject *
pytrap_init(PyObject *self, PyObject *args, PyObject *keywds)
{
    char **argv = NULL;
    char *arg;
    PyObject *argvlist;
    PyObject *strObj;
    int argc = 0, i, ifcin = 1, ifcout = 0;

    static char *kwlist[] = {"argv", "ifcin", "ifcout", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!|ii", kwlist, &PyList_Type, &argvlist, &ifcin, &ifcout))
        return NULL;
    argc = PyList_Size(argvlist);
    argv = calloc(argc, sizeof(char *));
    for (i=0; i<argc; i++) {
        strObj = PyList_GetItem(argvlist, i);
#if PY_MAJOR_VERSION >= 3
        arg = PyUnicode_AsUTF8AndSize(strObj, NULL);
#else
        arg = PyString_AS_STRING(strObj);
#endif
        argv[i] = arg;
    }

    int ret = local_trap_init(argc, argv, module_info, ifcin, ifcout);
    if (ret != 0) {
        PyErr_SetString(TrapError, "Initialization failed");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
pytrap_send(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"ifcidx", "data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IO!", kwlist, &ifcidx, &PyBytes_Type, &dataObj))
        return NULL;

    PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    if (data_size > 0xFFFF) {
        PyErr_SetString(TrapError, "Data length is out of range (0-65535)");
        return NULL;
    }
    int ret = trap_send(ifcidx, data, (uint16_t) data_size);
    if (ret == TRAP_E_TIMEOUT) {
        PyErr_SetString(TimeoutError, "Timeout");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
pytrap_recv(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    const void *in_rec;
    uint16_t in_rec_size;
    PyObject *data;

    static char *kwlist[] = {"ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i", kwlist, &ifcidx))
        return NULL;

    int ret = trap_recv(ifcidx, &in_rec, &in_rec_size);
    if (ret == TRAP_E_TIMEOUT) {
        PyErr_SetString(TimeoutError, "Timeout");
        return NULL;
    }
    data = PyBytes_FromStringAndSize(in_rec, in_rec_size);

    return data;
}

static PyObject *
pytrap_ifcctl(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t dir_in;
    uint32_t request;
    uint32_t ifcidx;
    uint32_t value;

    static char *kwlist[] = {"ifcidx", "dir_in", "request", "value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ipii", kwlist, &ifcidx, &dir_in, &request, &value))
        return NULL;

    trap_ifcctl((dir_in?TRAPIFC_INPUT:TRAPIFC_OUTPUT), ifcidx, request, value);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_terminate(PyObject *self, PyObject *args)
{
    printf("%s NOT IMPLEMENTED\n", __func__);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_finalize(PyObject *self, PyObject *args)
{
    TRAP_DEFAULT_FINALIZATION();
    // TODO FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
    ur_free_template(in_tmplt);
    ur_finalize();

    Py_RETURN_NONE;
}

static PyObject *
pytrap_sendFlush(PyObject *self, PyObject *args)
{
    printf("%s NOT IMPLEMENTED\n", __func__);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_setDataFmt(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    uint8_t data_type = TRAP_FMT_UNIREC;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", "data_type", "fmtspec", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|bs", kwlist, &ifcidx, &data_type, &fmtspec))
        return NULL;

    trap_set_data_fmt(ifcidx, data_type, fmtspec);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getDataFmt(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    uint8_t data_type;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i", kwlist, &ifcidx))
        return NULL;

    trap_get_data_fmt(TRAPIFC_INPUT, ifcidx, &data_type, &fmtspec);
    return Py_BuildValue("(iy)", data_type, fmtspec);
}

static PyObject *
pytrap_setVerboseLevel(PyObject *self, PyObject *args)
{
    int level = 0;

    if (!PyArg_ParseTuple(args, "i", &level))
        return NULL;

    trap_set_verbose_level(level);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getVerboseLevel(PyObject *self, PyObject *args)
{
    return PyLong_FromLong(trap_get_verbose_level());
}

static PyObject *
pytrap_setRequiredFmt(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    uint8_t data_type = TRAP_FMT_UNIREC;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", "data_type", "fmtspec", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|bs", kwlist, &ifcidx, &data_type, &fmtspec))
        return NULL;

    trap_set_required_fmt(ifcidx, data_type, fmtspec);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getInIFCState(PyObject *self, PyObject *args)
{
    printf("%s NOT IMPLEMENTED\n", __func__);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_createModuleInfo(PyObject *self, PyObject *args)
{
    printf("%s NOT IMPLEMENTED\n", __func__);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_updateModuleParam(PyObject *self, PyObject *args)
{
    printf("%s NOT IMPLEMENTED\n", __func__);

    Py_RETURN_NONE;
}

static PyObject *
pyunirec_timestamp(PyObject *self, PyObject *args)
{
    const char *bytes;
    int size;
    if (!PyArg_ParseTuple(args, "s#", &bytes, &size))
        return NULL;

    if (size == 8) {
        return Py_BuildValue("l", *((uint64_t *) bytes));
    }

    return NULL;
}

static PyObject *
pyunirec_ipaddr(PyObject *self, PyObject *args)
{
    const char *bytes;
    int size;
    ip_addr_t *ip;
    if (!PyArg_ParseTuple(args, "s#", &bytes, &size))
        return NULL;

    if (size == 16) {
        ip = (ip_addr_t *) bytes;
        if (ip_is4(ip)) {
            return Py_BuildValue("(I)", ip->ui32[2]);
        } else {
            return Py_BuildValue("(IIII)", ip->ui32[0], ip->ui32[1], ip->ui32[2], ip->ui32[3]);
        }
    }

    return NULL;
}

typedef struct {
    PyObject_HEAD
        trap_ctx_t *trap_ctx;
} pytrap_context;

static PyMethodDef pytrap_TrapContext_methods[] = {
    {"init",        (PyCFunction) pytrap_init, METH_VARARGS | METH_KEYWORDS,
        "Initialization of TRAP. init(argv, [ifcin, ifcout]), where argv is a list of strings, ifcin resp. ifcout is a number of input resp. output IFC."},

    {"recv",        (PyCFunction) pytrap_recv, METH_VARARGS | METH_KEYWORDS,
        "recv(ifcidx) -> bytes"},

    {"send",        (PyCFunction) pytrap_send, METH_VARARGS | METH_KEYWORDS,
        "send(ifcidx, bytes)"},

    {"ifcctl",      (PyCFunction) pytrap_ifcctl, METH_VARARGS | METH_KEYWORDS,
        "ifcctl(ifcidx, dir_in, request, value), where ifcidx is number, dir_in is bool - input direction of IFC, request: 1: TRAPCTL_AUTOFLUSH_TIMEOUT 2: TRAPCTL_BUFFERSWITCH 3: TRAPCTL_SETTIMEOUT, value is number"},

    {"setRequiredFmt",  (PyCFunction) pytrap_setRequiredFmt, METH_VARARGS | METH_KEYWORDS,
        "setRequiredFmt(ifcidx, [type, spec]), where ifcidx is number, type is int (0: TRAP_FMT_UNKNOWN, 1: TRAP_FMT_RAW, 2: TRAP_FMT_UNIREC), spec is string"},

    {"getDataFmt",  (PyCFunction) pytrap_getDataFmt, METH_VARARGS,
        "getDataFmt(ifcidx) -> (int, string)"},

    {"terminate",   pytrap_terminate, METH_VARARGS, "Terminate TRAP"},
    {"finalize",    pytrap_finalize, METH_VARARGS, "Free allocated memory"},
    {"sendFlush",   pytrap_sendFlush, METH_VARARGS | METH_KEYWORDS, ""},

    {"setDataFmt",  (PyCFunction) pytrap_setDataFmt, METH_VARARGS | METH_KEYWORDS,
        "setDataFmt(ifcidx, [type, spec]), where ifcidx is number, type is int (0: TRAP_FMT_UNKNOWN, 1: TRAP_FMT_RAW, 2: TRAP_FMT_UNIREC), spec is string"},

    {"setVerboseLevel", pytrap_setVerboseLevel, METH_VARARGS, "setVerboseLevel(int), where int is the verbose level."},
    {"getVerboseLevel", pytrap_getVerboseLevel, METH_VARARGS, "getVerboseLevel() -> int, current verbose level."},
    {"getInIFCState",   pytrap_getInIFCState, METH_VARARGS, ""},
    {"createModuleInfo",    pytrap_createModuleInfo, METH_VARARGS, ""},
    {"updateModuleParam",   pytrap_updateModuleParam, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject pytrap_TrapContext = {
    PyVarObject_HEAD_INIT(NULL, 0)
        "pytrap.TrapCtx",          /* tp_name */
    sizeof(pytrap_context),    /* tp_basicsize */
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
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "libtrap context",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_TrapContext_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};


static PyMethodDef pytrap_methods[] = {
    {"TimeStampFromUR",     pyunirec_timestamp, METH_VARARGS, "Convert RAW data to UniRec field (long)."},
    {"IPFromUR",    pyunirec_ipaddr, METH_VARARGS, "Convert RAW data to UniRec field (list of bytes)."},
    {NULL, NULL, 0, NULL}
};





#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef pytrapmodule = {
    PyModuleDef_HEAD_INIT,
    "pytrap",   /* name of module */
    "TRAP extension for python3.", /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    pytrap_methods, NULL, NULL, NULL, NULL
};

#  define INITERROR return NULL

PyMODINIT_FUNC
PyInit_pytrap(void)
#else
#  define INITERROR return

void
initpytrap(void)
#endif
{
    PyObject *m;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&pytrapmodule);
#else
    m = Py_InitModule("pytrap", pytrap_methods);
#endif
    if (m == NULL)
        INITERROR;

    pytrap_TrapContext.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pytrap_TrapContext) < 0)
        INITERROR;

    TimeoutError = PyErr_NewException("pytrap.TimeoutError", NULL, NULL);
    Py_INCREF(TimeoutError);
    PyModule_AddObject(m, "TimeoutError", TimeoutError);

    TrapError = PyErr_NewException("pytrap.error", NULL, NULL);
    Py_INCREF(TrapError);
    PyModule_AddObject(m, "error", TrapError);

    Py_INCREF(&pytrap_TrapContext);
    PyModule_AddObject(m, "TrapCtx", (PyObject *) &pytrap_TrapContext);
#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}

