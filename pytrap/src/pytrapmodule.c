#include <Python.h>
#include <structmember.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "pytrapexceptions.h"
#include "unirectemplate.h"

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

static PyObject *
pytrap_init(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
#if PY_MAJOR_VERSION >= 3
    const char *arg = NULL, *ifc_spec = NULL;
#else
    char *arg = NULL, *ifc_spec = NULL;
#endif
    char *module_name = "nemea-python-module", *module_desc = "",
         *service_ifcname = NULL;
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
    if (argc == 0) {
        PyErr_SetString(TrapError, "argv list must not be empty.");
        return NULL;
    }
    for (i = 0; i < argc; i++) {
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

    if (self->trap == NULL || trap_ctx_get_last_error(self->trap) != TRAP_E_OK) {
        PyErr_SetString(TrapError, "Initialization failed");
        return NULL;
    }

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
    } else if (PyUnicode_Check(dataObj)) {
        data = (char *) PyUnicode_AsUTF8AndSize(dataObj, &data_size);
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
pytrap_sendBulk(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    // IFC index
    uint32_t ifcidx = 0;
    int ret = 0;

    pytrap_unirectemplate *pyurtempl = NULL;
    PyObject *iterable = NULL;

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    static char *kwlist[] = {"urtempl", "data", "ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!O|I", kwlist, &pytrap_UnirecTemplate, &pyurtempl, &iterable, &ifcidx)) {
        return NULL;
    }


    if (!PySequence_Check(iterable)) {
        PyErr_SetString(PyExc_TypeError, "Data argument must be a sequence of dict().");
        return NULL;
    }

    Py_ssize_t count = PySequence_Size(iterable);
    if (count == -1) {
        PyErr_SetString(PyExc_IndexError, "Could not get size of iterable.");
        return NULL;
    } else if (count == 0) {
        Py_RETURN_NONE;
    }

    Py_ssize_t i;
    for (i = 0; i < count; i++) {
        PyObject *pydict = NULL;
        pydict = PySequence_GetItem(iterable, i);

        // Convert to dictionary
        UnirecTemplate_setFromDict(pyurtempl, pydict, 1 /* skip errors */);

        Py_BEGIN_ALLOW_THREADS
        ret = trap_ctx_send(self->trap, ifcidx, pyurtempl->data, pyurtempl->data_size);
        Py_END_ALLOW_THREADS
        Py_XDECREF(pydict);

        if (ret != TRAP_E_OK) {
            PyErr_Format(TrapError, "Sending failed: %s", trap_ctx_get_last_error_msg(self->trap));
            return NULL;
        }

    }

    Py_XDECREF(iterable);
    Py_RETURN_NONE;
}

static PyObject *
pytrap_recvBulk(pytrap_trapcontext *self, PyObject *args, PyObject *keywds)
{
    // IFC index
    uint32_t ifcidx = 0;
    // time (in seconds) to interrupt capturing
    uint32_t timeout = 60;
    // number of messages to interrupt capturing
    int32_t count = -1;

    const void *in_rec;
    uint16_t in_rec_size;

    pytrap_unirectemplate *pyurtempl = NULL;

    if (self->trap == NULL) {
        PyErr_SetString(TrapError, "TrapCtx is not initialized.");
        return NULL;
    }

    static char *kwlist[] = {"urtempl", "time", "count", "ifcidx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!Ii|I", kwlist, &pytrap_UnirecTemplate, &pyurtempl, &timeout, &count, &ifcidx)) {
        return NULL;
    }

    int ret;
    PyObject *pylist = PyList_New(0);
    PyObject *pydict = NULL;

    // get current time and stop time
    time_t endtime = time(NULL) + (time_t) timeout;

    while (endtime > time(NULL) && count != 0) {
        Py_BEGIN_ALLOW_THREADS
        ret = trap_ctx_recv(self->trap, ifcidx, &in_rec, &in_rec_size);
        Py_END_ALLOW_THREADS

        if (ret == TRAP_E_TIMEOUT || ret == TRAP_E_TERMINATED) {
            // nothing to read, return current data
            break;
        } else if (ret == TRAP_E_BAD_IFC_INDEX) {
            PyErr_SetString(TrapError, "Bad index of IFC.");
            goto error_cleanup;
        } else if (ret == TRAP_E_FORMAT_MISMATCH) {
            PyErr_SetString(TrapError, "Connection to incompatible IFC - format mismatch.");
            goto error_cleanup;
        } else if (ret == TRAP_E_FORMAT_CHANGED) {
            // recreate UnirecTemplate
            const char *spec;
            uint8_t data_type;
            trap_ctx_get_data_fmt(self->trap, TRAPIFC_INPUT, ifcidx, &data_type, &spec);
            pyurtempl->urtmplt = ur_define_fields_and_update_template(spec, pyurtempl->urtmplt);
            if (pyurtempl->urtmplt == NULL) {
                PyErr_SetString(TrapError, "Creation of UniRec template failed.");
                goto error_cleanup;
            }
            pyurtempl = UnirecTemplate_init(pyurtempl);
        }

        if (in_rec_size <= 1) {
            break;
        }
        if (count > 0) {
            count--;
        }

        // setData()
        pyurtempl->data = (void *) in_rec;
        pyurtempl->data_size = in_rec_size;
        if (pyurtempl->data_obj != NULL) {
            Py_DECREF(pyurtempl->data_obj);
        }
        pyurtempl->data_obj = PyByteArray_FromStringAndSize(in_rec, in_rec_size);;

        // Convert to dictionary
        pydict = UnirecTemplate_getDict(pyurtempl);

        if (pydict) {
            // Append into result list
            PyList_Append(pylist, pydict);

            Py_XDECREF(pydict);
        } else {
            /* if there was a problem with dict() construction, print
             * traceback and skip the message */
            PyErr_Print();
        }
    }

    return pylist;

error_cleanup:
    Py_DECREF(pylist);
    return NULL;
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
    in_tmplt = NULL;
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
    return Py_BuildValue("(is)", data_type, fmtspec);
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
        "    TimeoutError: Receiving data failed due to elapsed timeout.\n"
        "    TrapError: Bad index given.\n"
        "    FormatChanged: Data format was changed, it is necessary to\n"
        "        update template.  The received data is in `data` attribute\n"
        "        of the FormatChanged instance.\n"
        "    Terminated: The TRAP IFC was terminated.\n"},

    {"sendBulk",    (PyCFunction) pytrap_sendBulk, METH_VARARGS | METH_KEYWORDS,
        "Send sequence of records at once via TRAP interface.\n\n"
        "Example:\n"
        "    >>> import pytrap\n"
        "    >>> c1 = pytrap.TrapCtx()\n"
        "    >>> c1.init([\"-i\", \"f:/tmp/pytrap_sendbulktest\"], 0, 1)\n"
        "    >>> urtempl = \"ipaddr IP,uint16 PORT\"\n"
        "    >>> c1.setDataFmt(0, pytrap.FMT_UNIREC, urtempl)\n"
        "    >>> data = ({\"IP\": \"10.0.0.1\", \"PORT\": 1},\n"
        "    ...         {\"IP\": \"10.0.0.2\", \"PORT\": 2},\n"
        "    ...         {\"IP\": \"10.0.0.3\", \"PORT\": 3},\n"
        "    ...         {\"IP\": \"10.0.0.4\", \"PORT\": 4},\n"
        "    ...         {\"IP\": \"10.0.0.1\", \"PORT\": 5},\n"
        "    ...         {\"IP\": \"10.0.0.2\", \"PORT\": 6})\n"
        "    >>> t = pytrap.UnirecTemplate(urtempl)\n"
        "    >>> c1.sendBulk(t, data)\n"
        "    >>> c1.sendFlush()\n"
        "    >>> c1.finalize()\n\n"
        "Args:\n"
        "    urtempl (UnirecTemplate): Created UnirecTemplate with the template, it is updated internally when the format is changed.\n\n"
        "    data (list(dict)): Sequence of data to send; i.e., an iterable object containing dict, dict keys must match names of the UniRec fields in the template.\n\n"
        "    ifcidx (Optional[int]): Index of input IFC (default: 0).\n\n"
        "Raises:\n"
        "    TrapError: Bad index given.\n"},

    {"recvBulk",    (PyCFunction) pytrap_recvBulk, METH_VARARGS | METH_KEYWORDS,
        "Receive sequence of records at once via TRAP interface.\n\n"
        "Args:\n"
        "    urtempl (UnirecTemplate): Created UnirecTemplate with the template, it is updated internally when the format is changed.\n\n"
        "    time (int): Timeout in seconds before interrupt of capture.\n\n"
        "    count (int): Maximum number of messages to capture, infinite when -1 (Warning! This can consume much memory.).\n\n"
        "    ifcidx (Optional[int]): Index of input IFC (default: 0).\n\n"
        "Returns:\n"
        "    list(dict): Received data.\n\n"
        "Raises:\n"
        "    TrapError: Bad index given.\n"},

    {"send",        (PyCFunction) pytrap_send, METH_VARARGS | METH_KEYWORDS,
        "Send data via TRAP interface.\n\n"
        "Args:\n"
        "    bytes (bytearray or bytes): Data to send.\n\n"
        "    ifcidx (Optional[int]): Index of output IFC (default: 0).\n"
        "Raises:\n"
        "    TimeoutError: Receiving data failed due to elapsed timeout.\n"
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
        "    level (int): Level of verbosity the higher value the more verbose.\n"
        "                 Possible values: VERB_ERRORS (-3), VERB_WARNINGS (-2), VERB_NOTICES (-1) = default, VERB_VERBOSE (0), VERB_VERBOSE2 (1), VERB_VERBOSE3 (2).\n"
        },

    {"getVerboseLevel", (PyCFunction) pytrap_getVerboseLevel, METH_VARARGS,
        "Get current verbose level.\n\n"
        "Returns:\n"
        "    int: Level of verbosity. See setVerboseLevel() for possible values.\n"
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
    "TrapCtx()\n"
    "    Class represents libtrap context. It must be initialized using init()\n"
    "    and finalized and the end of usage finalize().\n"
    "    To terminate blocking libtrap functions use terminate().\n", /* tp_doc */
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
    PyType_GenericNew  /* tp_new */
};

PyObject *
pytrap_getTrapVersion(PyObject *self)
{
    PyObject *t = NULL;
    t = Py_BuildValue("(ss)",
                      trap_version,
                      trap_git_version);

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
"Examples:\n" \
"    >>> import pytrap\n" \
"    >>> c = pytrap.TrapCtx()\n" \
"    >>> c.init([\"-i\", \"u:socket1,u:socket2\"], 1, 1)\n" \
"    >>> fmttype = pytrap.FMT_UNIREC\n" \
"    >>> fmtspec = \"ipaddr SRC_IP\"\n" \
"    >>> c.setRequiredFmt(0, fmttype, fmtspec)\n" \
"    >>> rec = pytrap.UnirecTemplate(fmtspec)\n" \
"    >>> try:\n" \
"    ...     data = c.recv()\n" \
"    >>> except pytrap.FormatChanged as e:\n" \
"    ...     fmttype, fmtspec = c.getDataFmt(0)\n" \
"    ...     rec = pytrap.UnirecTemplate(fmtspec)\n" \
"    ...     data = e.data\n" \
"    >>> if len(data) <= 1:\n" \
"    ...     # empty message - do not process it!!!\n" \
"    ...     pass\n" \
"    >>> else:\n" \
"    ...     c.setDataFmt(0, fmttype, fmtspec)\n" \
"    ...     rec.setData(data)\n" \
"    ...     print(rec.strRecord())\n" \
"    >>> # send the message that was received:\n" \
"    >>> c.send(data)\n" \
"    >>> c.finalize()\n" \
"\n" \
"Simple example for data access using rec - UnirecTemplate instance::\n\n" \
"    >>> print(rec.SRC_IP)\n" \
"    >>> rec.SRC_IP = pytrap.UnirecIPAddr(\"127.0.0.1\")\n" \
"    >>> print(getattr(rec, \"SRC_IP\"))\n" \
"    >>> rec.TIME_FIRST = pytrap.UnirecTime(12345678)\n" \
"    >>> print(rec.TIME_FIRST)\n" \
"    >>> print(rec.TIME_FIRST.toDatetime())\n" \
"\n" \
"Simple example for creation of new message of UnirecTemplate::\n\n" \
"    >>> # createMessage() expects the maximal total size of fields with variable length as an argument,\n" \
"    >>> # here it is 100, i.e., size of all variable length data (sum of sizes) MUST be <= 100 bytes\n" \
"    >>> data = rec.createMessage(100)\n" \
"    >>> rec.DST_PORT = 80\n" \
"\n" \
"createMessage() should be called just at the beginning of program\n" \
"or when format change is needed.\n" \
"\n" \
"It is possible to set JSON format and send JSON documents via TRAP interface.\n\n" \
"Example - send::\n\n" \
"    >>> import pytrap\n" \
"    >>> import json\n" \
"    >>> c = pytrap.TrapCtx()\n" \
"    >>> c.init([\"-i\", \"f:/tmp/jsondata.trapcap:w\"], 0, 1)\n" \
"    >>> c.setDataFmt(0, pytrap.FMT_JSON, \"JSON\")\n" \
"    >>> a = json.dumps({\"a\": 123, \"b\": \"aaa\"})\n" \
"    >>> c.send(bytearray(a, \"utf-8\"))\n" \
"    >>> c.finalize()\n" \
"\n" \
"Example - receive::\n\n" \
"    >>> import pytrap\n" \
"    >>> import json\n" \
"    >>> c = pytrap.TrapCtx()\n" \
"    >>> c.init([\"-i\", \"f:/tmp/jsondata.trapcap\"], 1)\n" \
"    >>> c.setRequiredFmt(0, pytrap.FMT_JSON, \"JSON\")\n" \
"    >>> data = c.recv()\n" \
"    >>> print(json.loads(data.decode(\"utf-8\")))\n" \
"    >>> c.finalize()\n" \
"\n" \
"There are some complete example modules, see:\n" \
"https://github.com/CESNET/Nemea-Framework/tree/master/examples/python\n\n" \
"For more details, see the generated documentation:\n" \
"https://nemea.liberouter.org/doc/pytrap/.\n"

static struct PyModuleDef pytrapmodule = {
    PyModuleDef_HEAD_INIT,
    "pytrap.pytrap",   /* name of module */
    DOCSTRING_MODULE,
    -1,   /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    pytrap_methods, NULL, NULL, NULL, NULL
};

#  define INITERROR return NULL

PyMODINIT_FUNC
PyInit_pytrap(void)
{
    PyObject *m;

    m = PyModule_Create(&pytrapmodule);
    if (m == NULL) {
        INITERROR;
    }

    if (PyType_Ready(&pytrap_TrapContext) < 0) {
        INITERROR;
    }
    Py_INCREF(&pytrap_TrapContext);
    PyModule_AddObject(m, "TrapCtx", (PyObject *) &pytrap_TrapContext);

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

    PyModule_AddIntConstant(m, "VERB_ERRORS", -3);
    PyModule_AddIntConstant(m, "VERB_WARNINGS", -2);
    PyModule_AddIntConstant(m, "VERB_NOTICES", -1);
    PyModule_AddIntConstant(m, "VERB_VERBOSE",  0);
    PyModule_AddIntConstant(m, "VERB_VERBOSE2",  1);
    PyModule_AddIntConstant(m, "VERB_VERBOSE3",  2);

    return m;
}
