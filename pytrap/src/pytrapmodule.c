#include <Python.h>
#include <structmember.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "pytrapexceptions.h"

PyObject *TrapError;

PyObject *TimeoutError;

PyObject *TrapTerminated;

PyObject *TrapFMTChanged;

PyObject *TrapFMTMismatch;

PyObject *TrapHelp;

int init_unirectemplate(PyObject *m);

static ur_template_t *in_tmplt = NULL;

extern void *trap_glob_ctx;

typedef struct {
    PyObject_HEAD
    /**
     * Libtrap context
     */
    trap_ctx_t *trap;
} pytrap_trapcontext;

TRAP_DEFAULT_SIGNAL_HANDLER((void) 0)

static PyObject *
pytrap_init(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    char *arg, *module_name = "nemea-python-module", *module_desc = "",
         *service_ifcname = NULL, *ifc_spec = NULL;
    char service_name[20], found = 0, print_help = 0;
    PyObject *argvlist, *strObj;
    int argc = 0, i, ifcin = 1, ifcout = 0, result;

    static char *kwlist[] = {"argv", "ifcin", "ifcout", "module_name", "module_desc",
                             "service_ifcname", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!|iisss", kwlist, &PyList_Type, &argvlist,
                                     &ifcin, &ifcout, &module_name, &module_desc, &service_ifcname)) {
        return NULL;
    }

    argc = PyList_Size(argvlist);
    if (argc ==0) {
        PyErr_SetString(TrapError, "argv list must not be empty.");
        return NULL;
    }
    for (i=0; i<argc; i++) {
        strObj = PyList_GetItem(argvlist, i);
#if PY_MAJOR_VERSION >= 3
        result = PyUnicode_Check(strObj);
#else
        result = PyString_Check(strObj);
#endif
        if (!result) {
            PyErr_SetString(TrapError, "argv must contain string.");
            goto failure;
        }
#if PY_MAJOR_VERSION >= 3
        arg = PyUnicode_AsUTF8(strObj);
#else
        arg = PyString_AS_STRING(strObj);
#endif
        if (found == 1) {
            ifc_spec = arg;
            found = 0;
            continue;
        }
        if ((print_help == 1)) {
            if (!strcmp(arg, "1") || !strcmp(arg, "trap")) {
                trap_set_help_section(1);
            }
        } else if (strcmp(arg, "-i") == 0) {
            found = 1;
        } else if (!strncmp(arg, "-h", 2) || !strncmp(arg, "--help", 6)) {
            if (!strcmp(arg, "-h1") || !strcmp(arg, "--help=trap")) {
                trap_set_help_section(1);
            }
            print_help = 1;
        }
    }

    if (print_help) {
        trap_module_info_t module_info = {
            .name = module_name,
            .description = module_desc,
            .num_ifc_in =  ifcin,
            .num_ifc_out = ifcout,
            .params = NULL
        };
        trap_print_help(&module_info);
        PyErr_SetString(TrapHelp, "Printed help, skipped initialization.");
        return NULL;
    }

    if (ifc_spec == NULL) {
        PyErr_SetString(TrapError, "Missing -i argument.");
        return NULL;
    }

    if (service_ifcname == NULL) {
        snprintf(service_name, 20, "service_%d", getpid());
        service_ifcname = service_name;
    } else if (strlen(service_ifcname) == 0) {
        service_ifcname = NULL;
    }

    self->trap = trap_ctx_init3(module_name,
                                module_desc,
                                ifcin, ifcout,
                                ifc_spec,
                                service_ifcname);

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "Initialization failed");
        return NULL;
    }

    TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();

    Py_RETURN_NONE;
failure:
    return NULL;
}

static PyObject *
pytrap_send(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
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

    int ret;
    Py_BEGIN_ALLOW_THREADS
    ret = trap_ctx_send(self->trap, ifcidx, data, (uint16_t) data_size);
    Py_END_ALLOW_THREADS

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
pytrap_recv(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
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
    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    int ret;
    Py_BEGIN_ALLOW_THREADS
    ret = trap_ctx_recv(self->trap, ifcidx, &in_rec, &in_rec_size);
    Py_END_ALLOW_THREADS

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
    } else if (ret == TRAP_E_NOT_INITIALIZED) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
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
pytrap_ifcctl(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    PyObject *dir_in;
    uint32_t request;
    uint32_t ifcidx;
    uint32_t value;

    static char *kwlist[] = {"ifcidx", "dir_in", "request", "value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IO!ii", kwlist, &ifcidx, &PyBool_Type, &dir_in, &request, &value)) {
        return NULL;
    }

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    trap_ctx_ifcctl(self->trap, (PyObject_IsTrue((PyObject *) dir_in) ? TRAPIFC_INPUT : TRAPIFC_OUTPUT),
                ifcidx, request, value);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_terminate(pytrap_trapcontext *self, PyObject *args)
{
    trap_ctx_terminate(self->trap);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_finalize(pytrap_trapcontext *self, PyObject *args)
{
    TRAP_DEFAULT_FINALIZATION();
    trap_ctx_finalize(&self->trap);
    self->trap = NULL;
    ur_free_template(in_tmplt);
    ur_finalize();

    Py_RETURN_NONE;
}

static PyObject *
pytrap_sendFlush(pytrap_trapcontext *self, PyObject *args)
{
    uint32_t ifcidx = 0;

    if (!PyArg_ParseTuple(args, "|i", &ifcidx)) {
        return NULL;
    }

    trap_ctx_send_flush(self->trap, ifcidx);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_setDataFmt(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    uint8_t data_type = TRAP_FMT_UNIREC;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", "data_type", "fmtspec", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|bs", kwlist, &ifcidx, &data_type, &fmtspec)) {
        return NULL;
    }

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    trap_ctx_set_data_fmt(self->trap, ifcidx, data_type, fmtspec);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getDataFmt(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    uint8_t data_type;
    uint32_t ifcidx = 0;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|i", kwlist, &ifcidx)) {
        return NULL;
    }

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    trap_ctx_get_data_fmt(self->trap, TRAPIFC_INPUT, ifcidx, &data_type, &fmtspec);
    return Py_BuildValue("(is)", data_type, strdup(fmtspec));
}

static PyObject *
pytrap_setVerboseLevel(pytrap_trapcontext *self, PyObject *args)
{
    int level = 0;

    if (!PyArg_ParseTuple(args, "i", &level))
        return NULL;

    trap_set_verbose_level(level);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getVerboseLevel(pytrap_trapcontext *self, PyObject *args)
{
    return PyLong_FromLong(trap_get_verbose_level());
}

static PyObject *
pytrap_setRequiredFmt(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    uint32_t ifcidx;
    uint8_t data_type = TRAP_FMT_UNIREC;
    const char *fmtspec = "";

    static char *kwlist[] = {"ifcidx", "data_type", "fmtspec", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|bs", kwlist, &ifcidx, &data_type, &fmtspec)) {
        return NULL;
    }

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    trap_ctx_set_required_fmt(self->trap, ifcidx, data_type, fmtspec);

    Py_RETURN_NONE;
}

static PyObject *
pytrap_getInIFCState(pytrap_trapcontext *self, PyObject *args)
{
    uint32_t ifcidx = 0;

    if (!PyArg_ParseTuple(args, "i", &ifcidx))
        return NULL;

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    int state = trap_ctx_get_in_ifc_state(self->trap, ifcidx);
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
        "    ifcout (Optional[int]): `ifcout` is a number of output IFC (default: 0).\n"
        "    module_name (Optional[str]): Set name of this module.\n"
        "    module_desc (Optional[str]): Set description of this module.\n"
        "    service_ifcname (Optional[str]): Set identifier of service IFC, PID is used when omitted, set to \"\" (empty string) to disable service IFC.\n"
        "\n"
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

    {"terminate",   (PyCFunction) pytrap_terminate, METH_VARARGS,
        "Terminate TRAP."},

    {"finalize",    (PyCFunction) pytrap_finalize, METH_VARARGS,
        "Free allocated memory."},

    {"sendFlush",   (PyCFunction) pytrap_sendFlush, METH_VARARGS,
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

    {"setVerboseLevel", (PyCFunction) pytrap_setVerboseLevel, METH_VARARGS,
        "Set the verbose level of TRAP.\n\n"
        "Args:\n"
        "    level (int): Level of verbosity the higher value the more verbose (default: -1).\n"
        },

    {"getVerboseLevel", (PyCFunction) pytrap_getVerboseLevel, METH_VARARGS,
        "Get current verbose level.\n\n"
        "Returns:\n"
        "    int: Level of verbosity.\n"
        },

    {"getInIFCState",   (PyCFunction) pytrap_getInIFCState, METH_VARARGS,
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
    sizeof(pytrap_trapcontext), /* tp_basicsize */
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

PyObject *
pytrap_getTrapVersion(PyObject *self)
{
    PyObject *t = NULL;
    t = Py_BuildValue("(ss)",
                      strdup(trap_version),
                      strdup(trap_git_version));

    return t;
}

static PyMethodDef pytrap_methods[] = {
    {"getTrapVersion", (PyCFunction) pytrap_getTrapVersion, METH_NOARGS,
     "Get the version of libtrap.\n\n"
     "Returns:\n"
     "    tuple(str, str): libtrap version, git version.\n\n"},
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
"Simple example to receive and send one message:\n" \
"\n" \
"    import pytrap\n" \
"    c = pytrap.TrapCtx()\n" \
"    c.init([\"-i\", \"u:socket1,u:socket2\"], 1, 1)\n" \
"    fmttype = pytrap.FMT_UNIREC\n" \
"    fmtspec = \"ipaddr SRC_IP\"\n" \
"    c.setRequiredFmt(0, fmttype, fmtspec)\n" \
"    rec = pytrap.UnirecTemplate(fmtspec)\n" \
"    try:\n" \
"        data = c.recv()\n" \
"    except pytrap.FormatChanged as e:\n" \
"        fmttype, fmtspec = c.getDataFmt(0)\n" \
"        rec = pytrap.UnirecTemplate(fmtspec)\n" \
"        data = e.data\n" \
"    c.setDataFmt(0, fmttype, fmtspec)\n" \
"    rec.setData(data)\n" \
"    print(rec.strRecord())\n" \
"    # send the message that was received:\n" \
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
"createMessage() should be called just at the beginning of program\n" \
"or when format change is needed.\n\n" \
"There is a complete example module:\n" \
"https://github.com/CESNET/Nemea-Framework/tree/master/examples/python\n\n" \
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

    TrapHelp = PyErr_NewException("pytrap.TrapHelp", TrapHelp, NULL);
    Py_INCREF(TrapHelp);
    PyModule_AddObject(m, "TrapHelp", TrapHelp);

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

