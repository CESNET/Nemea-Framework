#ifndef UNIRECIPADDR_H
#define UNIRECIPADDR_H
#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    PyObject_HEAD
    ip_addr_t ip;
} pytrap_unirecipaddr;

PyAPI_DATA(PyTypeObject) pytrap_UnirecIPAddr;

typedef struct {
    PyObject_HEAD
    pytrap_unirecipaddr *start; /* low ip */
    pytrap_unirecipaddr *end;   /* high ip*/
} pytrap_unirecipaddrrange;

PyAPI_DATA(PyTypeObject) pytrap_UnirecIPAddrRange;

#ifdef __cplusplus
}
#endif
#endif /* UNIRECIPADDR_H */
