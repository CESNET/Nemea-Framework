#include <Python.h>
#include <structmember.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int init_unirectemplate(PyObject *m);

static trap_module_info_t *module_info = NULL;
static ur_template_t *in_tmplt = NULL;

#define MODULE_BASIC_INFO(BASIC) \
    BASIC("NEMEA module", "Module uses pytrap, it should handle help on its own.", 1, 0)

#define MODULE_PARAMS(PARAM)

TRAP_DEFAULT_SIGNAL_HANDLER((void) 0)

PyObject *TrapError;

static PyObject *TimeoutError;

static PyObject *TrapTerminated;

static PyObject *TrapFMTChanged;

static PyObject *TrapFMTMismatch;

static int
local_trap_init(int argc, char **argv, trap_module_info_t *module_info, int ifcin, int ifcout)
{
    INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
    module_info->num_ifc_in = ifcin;
    module_info->num_ifc_out = ifcout;

    TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);

    TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();

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
    uint32_t ifcidx = 0;
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", "ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|I", kwlist, &dataObj, &ifcidx)) {
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
    } else if (ret == TRAP_E_TERMINATED) {
        PyErr_SetString(TrapTerminated, "IFC was terminated.");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
pytrap_recv(PyObject *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx = 0;
    const void *in_rec;
    uint16_t in_rec_size;
    PyObject *data;
    PyObject *attr;

    static char *kwlist[] = {"ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|I", kwlist, &ifcidx)) {
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
    } else if (ret == TRAP_E_TERMINATED) {
        PyErr_SetString(TrapTerminated, "IFC was terminated.");
        return NULL;
    }
    data = PyByteArray_FromStringAndSize(in_rec, in_rec_size);
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
    PyObject *dir_in;
    uint32_t request;
    uint32_t ifcidx;
    uint32_t value;

    static char *kwlist[] = {"ifcidx", "dir_in", "request", "value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IO!ii", kwlist, &ifcidx, &PyBool_Type, &dir_in, &request, &value)) {
        return NULL;
    }

    trap_ifcctl((PyObject_IsTrue((PyObject *) dir_in) ? TRAPIFC_INPUT : TRAPIFC_OUTPUT),
                ifcidx, request, value);

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

    if (!PyArg_ParseTuple(args, "|i", &ifcidx)) {
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
    uint8_t data_type;
    uint32_t ifcidx = 0;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|i", kwlist, &ifcidx)) {
        return NULL;
    }

    trap_get_data_fmt(TRAPIFC_INPUT, ifcidx, &data_type, &fmtspec);
    return Py_BuildValue("(is)", data_type, strdup(fmtspec));
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

static PyMethodDef pytrap_TrapContext_methods[] = {
    {"init",        (PyCFunction) pytrap_init, METH_VARARGS | METH_KEYWORDS,
        "Initialization of TRAP.\n\n"
        "Args:\n"
        "    argv (list[str]): Arguments of the process.\n"
        "    ifcin (Optional[int]): `ifcin` is a number of input IFC (default: 1).\n"
        "    ifcout (Optional[int]): `ifcout` is a number of output IFC (default: 0).\n\n"
        "Raises:\n"
        "    TrapError: Initialization failed.\n"},

    {"recv",        (PyCFunction) pytrap_recv, METH_VARARGS | METH_KEYWORDS,
        "Receive data via TRAP interface.\n\n"
        "Args:\n"
        "    ifcidx (Optional[int]): Index of input IFC (default: 0).\n\n"
        "Returns:\n"
        "    bytearray: Received data.\n\n"
        "Raises:\n"
        "    TrapTimeout: Receiving data failed due to elapsed timeout.\n"
        "    TrapError: Bad index given.\n"
        "    FormatChanged: Data format was changed, it is necessary to\n"
        "        update template.  The received data is in `data` attribute\n"
        "        of the FormatChanged instance.\n"
        "    Terminated: The TRAP IFC was terminated.\n"},

    {"send",        (PyCFunction) pytrap_send, METH_VARARGS | METH_KEYWORDS,
        "Send data via TRAP interface.\n\n"
        "Args:\n"
        "    bytes (bytearray or bytes): Data to send.\n\n"
        "    ifcidx (Optional[int]): Index of output IFC (default: 0).\n"
        "Raises:\n"
        "    TrapTimeout: Receiving data failed due to elapsed timeout.\n"
        "    TrapError: Bad size or bad index given.\n"
        "    Terminated: The TRAP IFC was terminated.\n"},

    {"ifcctl",      (PyCFunction) pytrap_ifcctl, METH_VARARGS | METH_KEYWORDS,
        "Change settings of TRAP IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n"
        "    dir_in (bool): If True, input IFC will be modified, output IFC otherwise.\n"
        "    request (int): Type of request given by a module's constant (CTL_AUTOFLUSH, CTL_BUFFERSWITCH, CTL_TIMEOUT).\n"
        "    value (int): Parameter value of the chosen `request`.\n"},

    {"setRequiredFmt",  (PyCFunction) pytrap_setRequiredFmt, METH_VARARGS | METH_KEYWORDS,
        "Set required data format for input IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n"
        "    type (Optional[int]): Type of format: FMT_RAW, FMT_UNIREC (default: FMT_UNIREC)\n"
        "    spec (Optional[string]): Specifier of data format (UniRec specifier for FMT_UNIREC type) (default: \"\").\n"},

    {"getDataFmt",  (PyCFunction) pytrap_getDataFmt, METH_VARARGS | METH_KEYWORDS,
        "Get data format that was negotiated via input IFC.\n\n"
        "Args:\n"
        "    ifcidx (Optional[int]): Index of IFC (default: 0).\n\n"
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
        "    ifcidx (Optional[int]): Index of IFC (default: 0).\n\n"
        },

    {"setDataFmt",  (PyCFunction) pytrap_setDataFmt, METH_VARARGS | METH_KEYWORDS,
        "Set data format for output IFC.\n\n"
        "Args:\n"
        "    ifcidx (int): Index of IFC.\n"
        "    type (Optional[int]): Type of format (FMT_RAW, FMT_UNIREC), (default: FMT_UNIREC).\n"
        "    spec (Optional[string]): Specifier of data format (default: \"\").\n"
        },

    {"setVerboseLevel", pytrap_setVerboseLevel, METH_VARARGS,
        "Set the verbose level of TRAP.\n\n"
        "Args:\n"
        "    level (int): Level of verbosity the higher value the more verbose (default: -1).\n"
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

    {NULL, NULL, 0, NULL}
};

static PyTypeObject pytrap_TrapContext = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.TrapCtx", /* tp_name */
    0, /* tp_basicsize */
    0, /* tp_itemsize */
    0, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_reserved */
    0, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash  */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "libtrap context", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    pytrap_TrapContext_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    0, /* tp_init */
    0, /* tp_alloc */
    0  /* tp_new */
};

static PyMethodDef pytrap_methods[] = {
    {NULL, NULL, 0, NULL}
};

#define DOCSTRING_MODULE "TRAP extension for python3 (pytrap).\n\n" \
"This module can be used to write NEMEA module.  It consists of\n" \
"two main classes: *TrapCtx* and *UnirecTemplate*.  *TrapCtx*\n" \
"contains communication interface, actually, it is a wrapper\n" \
"for libtrap.  *UnirecTemplate* can be used for data access and\n" \
"manipulation.  It uses UniRec macros and functions in order to\n" \
"retrieve value of a field and to store value into data message.\n" \
"\n" \
"Simple example for receive and send messages:\n" \
"\n" \
"    import pytrap\n" \
"    c = pytrap.TrapCtx()\n" \
"    c.init([\"-i\", \"u:socket1,u:socket2\"], 1, 1)\n" \
"    fmtspec = \"ipaddr SRC_IP\"\n" \
"    c.setRequiredFmt(0, pytrap.FMT_UNIREC, fmtspec)\n" \
"    c.setDataFmt(0, pytrap.FMT_UNIREC, fmtspec)\n" \
"    rec = pytrap.UnirecTemplate(fmtspec)\n" \
"    try:\n" \
"        data = c.recv()\n" \
"    except pytrap.FormatChanged as e:\n" \
"        fmttype, fmtspec = c.getDataFmt(0)\n" \
"        c.setDataFmt(0, fmttype, fmtspec)\n" \
"        rec = pytrap.UnirecTemplate(fmtspec)\n" \
"        data = e.data\n" \
"    rec.setData(data)\n" \
"    print(rec.strRecord())\n" \
"    c.send(data)\n" \
"    c.finalize()\n" \
"\n" \
"Simple example for data access using rec - UnirecTemplate instance:\n" \
"\n" \
"    print(rec.SRC_IP)\n" \
"    rec.SRC_IP = pytrap.UnirecIPAddr(\"127.0.0.1\")\n" \
"    print(getattr(rec, \"SRC_IP\"))\n" \
"    rec.TIME_FIRST = pytrap.UnirecTime(12345678)\n" \
"    print(rec.TIME_FIRST)\n" \
"    print(rec.TIME_FIRST.toDatetime())\n" \
"\n" \
"Simple example for creation of new message of UnirecTemplate:\n" \
"\n" \
"    # 100 is the maximal total size of fields with variable length\n" \
"    data = rec.createMessage(100)\n" \
"    rec.DST_PORT = 80\n" \
"\n" \
"For more details, see docstring of the classes and methods.\n"

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef pytrapmodule = {
    PyModuleDef_HEAD_INIT,
    "pytrap",   /* name of module */
    DOCSTRING_MODULE,
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
    m = Py_InitModule3("pytrap", pytrap_methods, DOCSTRING_MODULE);
#endif
    if (m == NULL) {
        INITERROR;
    }

    pytrap_TrapContext.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pytrap_TrapContext) < 0) {
        INITERROR;
    }

    /* Add Exceptions into pytrap module */
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

    TrapTerminated = PyErr_NewException("pytrap.Terminated", TrapError, NULL);
    Py_INCREF(TrapFMTChanged);
    PyModule_AddObject(m, "Terminated", TrapTerminated);

    Py_INCREF(&pytrap_TrapContext);
    PyModule_AddObject(m, "TrapCtx", (PyObject *) &pytrap_TrapContext);

    /* Initialize UniRec part of pytrap */
    if (init_unirectemplate(m) == EXIT_FAILURE) {
        INITERROR;
    }

    /* Add constants into pytrap module */
    PyModule_AddIntConstant(m, "FMT_RAW", TRAP_FMT_RAW);
    PyModule_AddIntConstant(m, "FMT_UNIREC", TRAP_FMT_UNIREC);
    PyModule_AddIntConstant(m, "FMT_JSON", TRAP_FMT_JSON);

    PyModule_AddIntConstant(m, "FMTS_WAITING", FMT_WAITING);
    PyModule_AddIntConstant(m, "FMTS_OK", FMT_OK);
    PyModule_AddIntConstant(m, "FMTS_MISMATCH", FMT_MISMATCH);
    PyModule_AddIntConstant(m, "FMTS_CHANGED", FMT_CHANGED);

    PyModule_AddIntConstant(m, "CTL_AUTOFLUSH", TRAPCTL_AUTOFLUSH_TIMEOUT);
    PyModule_AddIntConstant(m, "CTL_BUFFERSWITCH", TRAPCTL_BUFFERSWITCH);
    PyModule_AddIntConstant(m, "CTL_TIMEOUT", TRAPCTL_SETTIMEOUT);

    PyModule_AddIntConstant(m, "TIMEOUT_WAIT", TRAP_WAIT);
    PyModule_AddIntConstant(m, "TIMEOUT_NOWAIT", TRAP_NO_WAIT);
    PyModule_AddIntConstant(m, "TIMEOUT_HALFWAIT", TRAP_HALFWAIT);
    PyModule_AddIntConstant(m, "TIMEOUT_NOAUTOFLUSH", TRAP_NO_AUTO_FLUSH);

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}

