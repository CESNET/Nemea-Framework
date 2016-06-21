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

static PyObject *TrapFMTChanged;

static PyObject *TrapFMTMismatch;

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
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!|ii", kwlist, &PyList_Type, &argvlist, &ifcin, &ifcout)) {
        return NULL;
    }

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
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IO!", kwlist, &ifcidx, &PyBytes_Type, &dataObj)) {
        return NULL;
    }

    PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    if (data_size > 0xFFFF) {
        PyErr_SetString(TrapError, "Data length is out of range (0-65535)");
        return NULL;
    }
    int ret = trap_send(ifcidx, data, (uint16_t) data_size);
    if (ret == TRAP_E_TIMEOUT) {
        PyErr_SetString(TimeoutError, "Timeout");
        return NULL;
    } else if (ret == TRAP_E_BAD_IFC_INDEX) {
        PyErr_SetString(TrapError, "Bad index of IFC.");
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
    PyObject *attr;

    static char *kwlist[] = {"ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i", kwlist, &ifcidx)) {
        return NULL;
    }

    int ret = trap_recv(ifcidx, &in_rec, &in_rec_size);
    if (ret == TRAP_E_TIMEOUT) {
        PyErr_SetString(TimeoutError, "Timeout");
        return NULL;
    } else if (ret == TRAP_E_BAD_IFC_INDEX) {
        PyErr_SetString(TrapError, "Bad index of IFC.");
        return NULL;
    } else if (ret == TRAP_E_FORMAT_MISMATCH) {
        PyErr_SetString(TrapFMTMismatch, "Format mismatch, incompatible data format of sender and receiver.");
        return NULL;
    }
    data = PyBytes_FromStringAndSize(in_rec, in_rec_size);
    if (ret == TRAP_E_FORMAT_CHANGED) {
        attr = Py_BuildValue("s", "data");
        PyObject_SetAttr(TrapFMTChanged, attr, data);
        PyErr_SetString(TrapFMTChanged, "Format changed.");
        return NULL;
    }
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
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ipii", kwlist, &ifcidx, &dir_in, &request, &value)) {
        return NULL;
    }

    trap_ifcctl((dir_in?TRAPIFC_INPUT:TRAPIFC_OUTPUT), ifcidx, request, value);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_terminate(PyObject *self, PyObject *args)
{
    trap_terminate();

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
    uint32_t ifcidx = 0;

    if (!PyArg_ParseTuple(args, "i", &ifcidx)) {
        return NULL;
    }

    trap_send_flush(ifcidx);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_setDataFmt(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    uint8_t data_type = TRAP_FMT_UNIREC;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", "data_type", "fmtspec", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|bs", kwlist, &ifcidx, &data_type, &fmtspec)) {
        return NULL;
    }

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
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|bs", kwlist, &ifcidx, &data_type, &fmtspec)) {
        return NULL;
    }

    trap_set_required_fmt(ifcidx, data_type, fmtspec);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getInIFCState(PyObject *self, PyObject *args)
{
    printf("%s NOT IMPLEMENTED\n", __func__);
    uint32_t ifcidx = 0;

    if (!PyArg_ParseTuple(args, "i", &ifcidx))
        return NULL;

    int state = trap_get_in_ifc_state(ifcidx);
    if (state == TRAP_E_BAD_IFC_INDEX) {
        PyErr_SetString(TrapError, "Bad index of IFC.");
        return NULL;
    } else if (state == TRAP_E_NOT_INITIALIZED) {
        PyErr_SetString(TrapError, "Not initialized.");
        return NULL;
    }

    return PyLong_FromLong(state);
}

/*
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
*/

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
        "Initialization of TRAP.\n\n"
        "Args:\n"
        "    argv (list[str]): Arguments of the process.\n"
        "    ifcin (Optional[in]): `ifcin` is a number of input IFC.\n"
        "    ifcout (Optional[in]): `ifcout` is a number of output IFC.\n\n"
        "Raises:\n"
        "    TrapError: Initialization failed.\n"},

    {"recv",        (PyCFunction) pytrap_recv, METH_VARARGS | METH_KEYWORDS,
        "Receive data via TRAP interface.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of input IFC.\n\n"
        "Returns:\n"
        "    bytes: Received data.\n\n"
        "Raises:\n"
        "    TrapTimeout: Receiving data failed due to elapsed timeout.\n"
        "    TrapError: Bad index given.\n"
        "    FormatChanged: Data format was changed, it is necessary to\n"
        "        update template.  The received data is in `data` attribute\n"
        "        of the FormatChanged instance.\n"},

    {"send",        (PyCFunction) pytrap_send, METH_VARARGS | METH_KEYWORDS,
        "Send data via TRAP interface.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of output IFC.\n"
        "    bytes (bytes): Data to send.\n\n"
        "Raises:\n"
        "    TrapTimeout: Receiving data failed due to elapsed timeout.\n"
        "    TrapError: Bad size or bad index given.\n"},

    {"ifcctl",      (PyCFunction) pytrap_ifcctl, METH_VARARGS | METH_KEYWORDS,
        "Change settings of TRAP IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n"
        "    dir_in (bool): If True, input IFC will be modified, output IFC otherwise.\n"
        "    request (int): Type of request given by a module's constant (CTL_AUTOFLUSH, CTL_BUFFERFLUSH, CTL_TIMEOUT).\n"
        "    value (int): Parameter value of the chosen `request`.\n"},

    {"setRequiredFmt",  (PyCFunction) pytrap_setRequiredFmt, METH_VARARGS | METH_KEYWORDS,
        "Set required data format for input IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n"
        "    type (int): Type of format given by a module's constant (FMT_RAW, FMT_UNIREC)\n"
        "    spec (string): Specifier of data format (e.g. UniRec specifier for FMT_UNIREC).\n"},

    {"getDataFmt",  (PyCFunction) pytrap_getDataFmt, METH_VARARGS,
        "Get data format that was negotiated via input IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n\n"
        "Returns:\n"
        "    Tuple(int, string): Type of format and specifier (see setRequiredFmt()).\n\n"
        },

    {"terminate",   pytrap_terminate, METH_VARARGS,
        "Terminate TRAP."},

    {"finalize",    pytrap_finalize, METH_VARARGS,
        "Free allocated memory."},

    {"sendFlush",   pytrap_sendFlush, METH_VARARGS,
        "Force sending buffer for IFC with ifcidx index.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n\n"
        },

    {"setDataFmt",  (PyCFunction) pytrap_setDataFmt, METH_VARARGS | METH_KEYWORDS,
        "Set data format for output IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n"
        "    type (Optional[int]): Type of format (FMT_RAW, FMT_UNIREC), FMT_UNIREC if missing.\n"
        "    spec (Optional[string]): Specifier of data format, \"\" if missing.\n"
        },

    {"setVerboseLevel", pytrap_setVerboseLevel, METH_VARARGS,
        "Set the verbose level of TRAP.\n\n"
        "Args:\n"
        "    level (int): Level of verbosity, 0 by default, the higher value the more verbose.\n"
        },

    {"getVerboseLevel", pytrap_getVerboseLevel, METH_VARARGS,
        "Get current verbose level.\n\n"
        "Returns:\n"
        "    int: Level of verbosity.\n"
        },

    {"getInIFCState",   pytrap_getInIFCState, METH_VARARGS,
        "Get the state of input IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n\n"
        "Returns:\n"
        "    int: One of module's constants (FMTS_WAITING, FMTS_OK, FMTS_MISMATCH, FMTS_CHANGED).\n\n"
        "Raises:\n"
        "    TrapError: Bad index is passed or TRAP is not initialized.\n"
        },

    /*
    {"createModuleInfo",    pytrap_createModuleInfo, METH_VARARGS, ""},
    {"updateModuleParam",   pytrap_updateModuleParam, METH_VARARGS, ""},
    */
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

    TrapError = PyErr_NewException("pytrap.TrapError", NULL, NULL);
    Py_INCREF(TrapError);
    PyModule_AddObject(m, "TrapError", TrapError);

    TimeoutError = PyErr_NewException("pytrap.TimeoutError", TrapError, NULL);
    Py_INCREF(TimeoutError);
    PyModule_AddObject(m, "TimeoutError", TimeoutError);

    TrapFMTChanged = PyErr_NewException("pytrap.FormatChanged", TrapError, NULL);
    Py_INCREF(TrapFMTChanged);
    PyModule_AddObject(m, "FormatChanged", TrapFMTChanged);

    TrapFMTMismatch = PyErr_NewException("pytrap.FormatMismatch", TrapError, NULL);
    Py_INCREF(TrapFMTChanged);
    PyModule_AddObject(m, "FormatMismatch", TrapFMTMismatch);

    Py_INCREF(&pytrap_TrapContext);
    PyModule_AddObject(m, "TrapCtx", (PyObject *) &pytrap_TrapContext);

    PyModule_AddIntConstant(m, "FMT_RAW", TRAP_FMT_RAW);
    PyModule_AddIntConstant(m, "FMT_UNIREC", TRAP_FMT_UNIREC);

    PyModule_AddIntConstant(m, "FMTS_WAITING", FMT_WAITING);
    PyModule_AddIntConstant(m, "FMTS_OK", FMT_OK);
    PyModule_AddIntConstant(m, "FMTS_MISMATCH", FMT_MISMATCH);
    PyModule_AddIntConstant(m, "FMTS_CHANGED", FMT_CHANGED);

    PyModule_AddIntConstant(m, "CTL_AUTOFLUSH", TRAPCTL_AUTOFLUSH_TIMEOUT);
    PyModule_AddIntConstant(m, "CTL_BUFFERFLUSH", TRAPCTL_BUFFERSWITCH);
    PyModule_AddIntConstant(m, "CTL_TIMEOUT", TRAPCTL_SETTIMEOUT);




#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}

