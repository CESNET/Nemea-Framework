#ifndef UNIRECTEMPLATE_H
#define UNIRECTEMPLATE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    ur_template_t *urtmplt;
    char *data;
    Py_ssize_t data_size;
    PyObject *data_obj; // Pointer to object containing the data we are pointing to
    PyDictObject *urdict;
    PyDictObject *fields_dict; // dictionary of field names indexed by field ID

    /* for iteration */
    Py_ssize_t iter_index;
    Py_ssize_t field_count;
} pytrap_unirectemplate;

PyAPI_DATA(PyTypeObject) pytrap_UnirecTemplate;

PyAPI_FUNC(PyObject *) UnirecTemplate_getAttr(pytrap_unirectemplate *self, PyObject *attr);

PyAPI_FUNC(PyObject *) UnirecTemplate_setData(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds);

PyAPI_FUNC(PyObject *) UnirecTemplate_getDict(pytrap_unirectemplate *self);

PyAPI_FUNC(PyObject *) UnirecTemplate_createMessage(pytrap_unirectemplate *self, uint32_t dynamic_size);

PyAPI_FUNC(PyObject *) UnirecTemplate_setFromDict(pytrap_unirectemplate *self, PyObject *dict, int skip_errors);

PyAPI_FUNC(pytrap_unirectemplate *) UnirecTemplate_init(pytrap_unirectemplate *self);

#ifdef __cplusplus
}
#endif
#endif

