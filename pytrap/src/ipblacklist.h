#ifndef UNIRECIPLIST_H
#define UNIRECIPLIST_H
#ifdef __cplusplus
extern "C" {
#endif

#include <unirec/ip_prefix_search.h>

typedef struct {
    PyObject_HEAD
    ipps_context_t *ipps_ctx;
} pytrap_unireciplist;

PyAPI_DATA(PyTypeObject) pytrap_UnirecIPList;

#ifdef __cplusplus
}
#endif
#endif /* UNIRECIPADDR_H */

