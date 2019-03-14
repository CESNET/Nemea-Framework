#ifndef UNIRECMACADDR_H
#define UNIRECMACADDR_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    mac_addr_t mac;
} pytrap_unirecmacaddr;

PyAPI_DATA(PyTypeObject) pytrap_UnirecMACAddr;

typedef struct {
    PyObject_HEAD
    pytrap_unirecmacaddr *start; /* low mac */
    pytrap_unirecmacaddr *end;   /* high mac*/
} pytrap_unirecmacaddrrange;

PyAPI_DATA(PyTypeObject) pytrap_UnirecMACAddrRange;

#ifdef __cplusplus
}
#endif
#endif /* UNIRECMACADDR_H */
