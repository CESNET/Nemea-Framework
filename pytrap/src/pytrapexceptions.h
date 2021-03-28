#ifndef PYTRAPEXCEPTIONS_H
#define PYTRAPEXCEPTIONS_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyObject *) TrapError;

PyAPI_DATA(PyObject *) TimeoutError;

PyAPI_DATA(PyObject *) TrapTerminated;

PyAPI_DATA(PyObject *) TrapFMTChanged;

PyAPI_DATA(PyObject *) TrapFMTMismatch;

PyAPI_DATA(PyObject *) TrapHelp;

#ifdef __cplusplus
}
#endif
#endif /* PYTRAPEXCEPTIONS_H */
