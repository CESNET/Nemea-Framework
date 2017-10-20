UniRec
=======

Overview
--------
UniRec is a data format for storage and transfer of simple unstructured records,
i.e. sets of key-value pairs with a fixed set of keys. A record in UniRec format
is similar to a C structure but it can be defined at run-time. It thus brings
possibility to dynamically create structures in a statically typed language.

The main advantage of UniRec is extremely fast access to fields of a record.
No parsing is needed, the fields are accessed directly from the record, almost
as in a plain C struct.

In comparison with access to a struct member, just one additional memory
access is needed in order to find position of the field in the record. This
access is to a small table which easily fits into a CPU cache.

To create an UniRec record, a user first needs to specify a set of fields and
their types - a template. Then a memory for the record is allocated and field
values can be set using simple macros.

NOTE: The following text describes UniRec as used in the C or C++ language.

### Simplified example:

```
// Create a template with three fields (their types must be defined earlier)
ur_template_t *tmplt = ur_create_template("FIELD1,FIELD2,FIELD3", NULL);

// Create a record with that template
void *record = ur_create_record(tmplt, 0);

// Set values of fields
ur_set(tmplt, record, F_FIELD1, 1);
ur_set(tmplt, record, F_FIELD2, 234);
ur_set(tmplt, record, F_FIELD3, 56);

// Read values of the record and print them to standard output
printf("%i %i %i\n",
   ur_get(tmplt, record, F_FIELD1),
   ur_get(tmplt, record, F_FIELD2),
   ur_get(tmplt, record, F_FIELD3),
);
```

The example states that the types of the fields must be defined before a
template can be created. If names and types of the fields are known at
compile-time, they can be defined at the beginning of a *.c file as in the
following example:
```
// Specify which fields will be used in the code and what are their types
UR_FIELDS(
   int32 FIELD1,
   int32 FIELD2,
   int32 FIELD3,
)
```
If the set of fields and their types is not known in advance, they may also be
defined at run-time. However, access to such fields is then a little more
complicated due to limitations of statically types languages (if a compiler
doesn't know the type of a field, it can't create a set of instructions to read
from or write into it).


UniRec data types
-----------------

An UniRec field may have one of the following types:

|name    |size|  description                                                 |
|--------|----|--------------------------------------------------------------|
|int8    |  1 |8bit singed integer                                           |
|int16   |  2 |16bit singed integer                                          |
|int32   |  4 |32bit singed integer                                          |
|int64   |  8 |64bit singed integer                                          |
|uint8   |  1 |8bit unsigned integer                                         |
|uint16  |  2 |16bit unsigned integer                                        |
|uint32  |  4 |32bit unsigned integer                                        |
|uint64  |  8 |64bit unsigned integer                                        |
|char    |  1 |A single ASCII character                                      |
|float   |  4 |Single precision floating point number (IEEE 754)             |
|double  |  8 |Double precision floating point number (IEEE 754)             |
|ipaddr  | 16 |Special type for IPv4/IPv6 addresses, see below for details   |
|macaddr |  6 |Special type for MAC address, see below for details           |
|time    |  8 |Special type for precise timestamps, see below for details    |
|string  |  - |Variable-length array of (mostly) printable characters        |
|bytes   |  - |Variable-length array of bytes (not expected to be printable characters) |


Types "string" and "bytes" are the same from a machine point of view (both are
of type char[] in C), the only difference is their semantics. When printing
as text, "string" is usually printed directly as ASCII or UTF-8 string,
"bytes" is rather interpreted as binary data and printed in hex.

A terminating null character ('\0') SHOULD NOT be included at the end of
"string" values since this is specific for the C language and data in UniRec
should be independent of a programming language.

### ipaddr type
Structure to store both IPv4 and IPv6 addresses and associated functions.

### macaddr type
Structure to store MAC address and associated functions.

### time type
Structure to store timestamps and associated types, macros and function.


Field names
-----------

Name of field may be any string matching the regular expression
  [A-Za-z][A-Za-z0-9_]*
with the following limitations:
  - It SHOULD NOT end with "_T" as this is reserved in C implementation for
    symbolic constants storing the type of a field.

It is RECOMMENDED that all field names are uppercase.

Physical record layout
----------------------

An UniRec record consists of field values put one after another in a specific
order. There is no header. Information about the template and the size of the
record must be provided by other means.

The layout of a record is given only by its template (specifying a set of fields
and their types) and the following rules.

A record is divided into three sections:
  1. Values of all fixed-length fields
  2. Meta-information about variable-length fields
  3. Data of variable-length fields

Fixed-length fields in the first section are sorted by their size from largest
to smallest. Fields with the same size are sorted alphabetically by their name.

The second section contains two 16bit numbers for each variable-length field -
offset of the beginning of the field's data and length of the data (in bytes).
The offset is counted from the beginning of the record.

The meta-information fields are sorted alphabetically by the field names.

The third section contains data of variable-length fields in an arbitrary order.
The data of variable-length fields SHOULD be placed immediately one after
another. There SHOULD be NO "empty spaces" between them and data of the fields
SHOULD NOT overlap.

The first two sections are called the "fixed-length part" of a record, since their
total size is always the same and all data are present on fixed offsets (for a
given template). The last section is called "variable-length part" because its total
length as well as position of individual fields may be different in each record.

### Example

The following picture shows layout of a record containing information about a
HTTP connection. The template of this record contains the following fields:
  ipaddr SRC_IP, ipaddr DST_IP, uint16 SRC_PORT, uint16 DST_PORT,
  uint8 PROTOCOL, uint8 TCP_FLAGS, uint32 PACKETS, uint32 BYTES,
  uint16 HTTP_RSP_CODE, string HTTP_URL, string HTTP_USER_AGENT

```
byte      0       1       2       3
      +-------+-------+-------+-------+
 0    |                               |
 4    |            DST_IP             |
 8    |                               |
12    |                               |
      +-------+-------+-------+-------+
16    |                               |
20    |            SRC_IP             |
24    |                               |
28    |                               |
      +-------+-------+-------+-------+
32    |             BYTES             |
      +-------+-------+-------+-------+
36    |            PACKETS            |
      +-------+-------+-------+-------+
40    |   DST_PORT    | HTTP_RSP_CODE |
      +-------+-------+-------+-------+
44    |   SRC_PORT    | PROTO | TCP_F |
      +-------+-------+-------+-------+
48    | HTTP_URL(off) | HTTP_URL(len) |
      +-------+-------+-------+-------+
52    | HTTP_USER(off)| HTTP_USER(len)|   fixed-length
      +-------+-------+-------+-------+   -----------------
56    |         HTTP_URL (data)       |   variable-length part
      +                       +-------+
60    |                       |       |
      +-------+-------+-------+       +
64    |     HTTP_USER_AGENT (data)    |
      +               +-------+-------+
68    |               |
      +-------+-------+
```


### Endianness

All values, except IP and MAC addresses, are in little endian. IP and MAC addresses are treated
rather as sequences of bytes than numbers, so they are left in network order,
i.e. big-endian (however, they are encapsulated in a special data type and
shouldn't be accessed directly so the internal format should be needed to know).


### Maximal record length

Maximal length of the record is limited to 65534 (2^16 - 2) bytes.


### Template definition

Templates are usually defined by a string enumerating all the fields in the
template, using comma (',') as a separator of field names. Order of field names
in such string is not important (since physical order of fields is given by the
rules above).

C library interface
===================

Types and structures
--------------------

Types, enums and structures defined in unirec.h.
```
enum ur_field_type {
   UR_TYPE_INT8,
   UR_TYPE_INT16,
   UR_TYPE_INT32,
   UR_TYPE_INT64,
   UR_TYPE_UINT8,
   UR_TYPE_UINT16,
   UR_TYPE_UINT32,
   UR_TYPE_UINT64,
   UR_TYPE_CHAR,
   UR_TYPE_FLOAT,
   UR_TYPE_DOUBLE,
   UR_TYPE_IP,
   UR_TYPE_MAC,
   UR_TYPE_TIME,
   UR_TYPE_STRING,
   UR_TYPE_BYTES,
};
```
  An enum value for each of the UniRec types.

#### ur_field_id_t
Unsigned integer type for holding field IDs.
IMPLEMENTATION NOTE: ur_field_id_t = uint16_t

#### ur_template_t
A structure defining an UniRec template. It contains information about which
fields are present in a record with that template and how to access them.
For user this is a black box, it is not needed to access the structure's
members directly.

#### ur_iter_t
Iterator type used by ur_iter_fields function

#### UR_ITER_BEGIN
#### UR_ITER_END
Constants used for iteration over fields in a template, see ur_iter_fields
function for details.


#### UR_MAX_SIZE = 65535
   Maximal size of an UniRec record.

Public functions and macros
---------------------------

### Definition of statically-known UniRec fields.
```
UR_FIELDS(type name [, type name [, ...] ])
```
This macro allows to define fields used in the program and their types at
compile-time. This allows to access such fields in UniRec records more easily
and efficiently.

This macro should be used in the beginning of each translation unit (i.e. a *.c
file) if the fields used (or at least some of them) are known at compile-time.

Parameters:
- "name" may be any string matching the following regular expression:
  [A-Za-z][A-Za-z0-9_]*
  with the following exceptions:
  - It must not be the same as a keyword in C/C++ or another identifier used in the source codes.
    To avoid collisions with other identifiers in the UniRec library, do not use
    identifiers beginning with "UR\_" or "ur\_"
  - It must not end with _T (as this is reserved for constants specifying types)
  - It is RECOMMENDED that all field names are upper case.
- "type" is one of the types specified in "format specification - data types".

There MAY be a comma after the last field name. Also, there MAY be a semicolon
after the closing parenthesis at the end of the macro.

#### Example:
```
UR_FIELDS(
  ipaddr   SRC_IP,
  ipaddr   DST_IP,
  uint16   SRC_PORT,
  uint16   DST_PORT,
  uint8    PROTO,
)
```
This macro generates code allowing to use the defined fields in ur_get, ur_set
and other macros which need symbolic constants to access the fields.

For each field specified by this macro, a CPP macro is defined with `F_` prefix
in the name and a value of a unique numeric ID.  Also, a constant F_name_T is
defined with a value of the field's type (as defined in ur_field_type enum).
Other internal variables and structures are defined.

If there are more than one translation unit accessing UniRec fields, the same set
of fields MUST be defined using UR_FIELDS in each of them.


### Cleanup of all internal structures.
```
int ur_finalize()
```
This function has to be called after all UniRec functions and macros
invocations if there were some fields defined at run-time. Otherwise this function
does not have any effect, because nothing has been allocated. The function is called
typically during a cleanup phase before the program's end.

No UniRec function or macro can be called after a call to ur_finalize.


### Run-time definition of a field
```
int ur_define_field(const char *name, ur_field_type type)
```
This function allows to define a field at run-time. 

Parameters:
"name" - name of the new field, see description of UR_FIELDS for rules on
field names.
"type" - type of the new field.

If a field with the same name already exists in the internal table of defined
fields and "type" is the same as the one in the table, the function just returns
the ID of the field. If types does not match, a UR_E_TYPE_MISMATCH error code
is returned.

If no field with "name" is present in the table of fields, a new entry is
created with a new unique ID and the given name and type of the field.
The new ID is returned.

Returns:
- ID of a field with the given name if no error occurs.
- UR_E_TYPE_MISMATCH if a field with the given name is already defined with a
  different type.
- UR_E_INVALID_NAME if the name is not a valid field name.
- UR_E_INVALID_TYPE if the type is not one of the values of enum ur_field_type.
- UR_E_MEMORY if memory allocation error occurred.

All error codes returned by this function are negative integers, ID is always
non-negative.

If this function is used in a program, the function ur_finalize() has to be
called after all UniRec functions and macros invocations.

NOTE: It is not necessary to define fields which were defined by UR_FIELDS.
It is recommended to define all fields statically by UR_FIELDS if possible.
This function is present only for cases when field names and/or types are not
known until run-time.

NOTE: Fields defined by this function can be accessed using their numeric IDs
only. Symbolic CPP macros are not defined, of course.


### Run-time definition of a set of fields

```
int ur_define_set_of_fields(const char *ifc_data_fmt);
```

This function allows to define sef of fields at run-time.

Define new UniRec fields at run-time. It adds new fields into existing structures.
If the field already exists and type is equal nothing will happen. If the type is not equal
an error will be returned.

Parameters:
"fc_data_fmt" - String containing types and names of fields delimited by comma.
Example ifc_data_fmt: "uint32 FOO,uint8 BAR,float FOO2"


Returns:
- UR_OK on success
- UR_E_MEMORY if there is an allocation problem.
- UR_E_INVALID_NAME if the name value is empty.
- UR_E_INVALID_TYPE if the type does not exist.
- UR_E_TYPE_MISMATCH if the name already exists, but the type is different.


### Undefine field
```
int ur_undefine_field(const char *name)
int ur_undefine_field_by_id(ur_field_id_t id)
```
Allows to revert a previous definition of a field by ur_define_field.

Frees the ID of the given field for future re-use. The ID becomes invalid after
a call to this function so the field with the given name can not be accessed
any more. Note that the same ID may be assigned to another field later.

This function is not necessary in most cases. Its only purpose is to allow a
re-use of field IDs since their total count is limited to 2^16-1.

After this function is used, all the templates using the undefined field have to
freed and created again.


### Create UniRec template
```
ur_template_t *ur_create_template(const char* fields, char **errstr)
```
Creates a structure describing an UniRec template with the given set of fields
and returns a pointer to it.

The template should be freed by ur_free_template after is not needed any more.

Parameters:
- "fields" - A string containing names of fields separated by commas, e.g.:
"FOO,BAR,BAZ"
- "errstr" - (output) In case of an error a pointer to the error message is
returned using this parameter, if not set to NULL.

Order of field names is not important, i.e. any two strings with the same set of
field names but with different order are equivalent.

All fields MUST be previously defined, either statically by UR_FIELDS or by
calls to ur_define_field.

If an error occurs and "errstr" is not NULL, it is set to a string with
corresponding error message.


Returns:
- Pointer to the newly created template or NULL if an error has occurred.


### Create UniRec template for usage with Libtrap
```
ur_template_t *ur_create_input_template(int ifc, const char* fields, char **errstr)
```
Creates UniRec template and set this template to specified input interface (ifc).

This template will be set as a minimum set of fields to be able to receive messages.
If the input interface receives superset of fields, the template will be expanded.

```
ur_template_t *ur_create_output_template(int ifc, const char* fields, char **errstr)
```
Creates UniRec template and set this template to specified output interface (ifc).

Set of fields of this template will be set to an output interface.

```
ur_template_t *ur_ctx_create_bidirectional_template(trap_ctx_t *ctx, int ifc_in, int ifc_out, const char* fields, char **errstr)
```
Creates UniRec template and set this template to specified input (ifc_in) and output (ifc_out) interface.

This template will be set as a minimum set of fields to be able to receive messages.
If the input interface receives superset of fields, the template will be expanded and
new set of fields will be set to output interface.


### Free UniRec template
```
void *ur_free_template(ur_template_t *tmplt)
```
Free memory allocated for a template.



### Retrieve value from UniRec record.
Following functions are used to retrieve certain field value from UniRec record.

Parameters:
- "tmplt" - Pointer to UniRec template
- "rec" - Pointer to UniRec record, which is created using given template.
- "field" - Identifier of a field.

```
ur_get(tmplt, rec, field)
```
This function returns value of an appropriate type of a specific field (int, uint, ...).
Because of this, the field must be a symbolic constant (i.e. "F_name") not a numerical ID.
It can be used just for fixed size fields (not for string and bytes).

```
ur_get_ptr(tmplt, rec, field)
```
This function returns pointer to a value of an appropriate type. Because of this,
the field must be a symbolic constant (i.e. "F_name") not a numerical ID.
It can be used for both fixed-length and variable-length fields.

```
ur_get_ptr_by_id(tmplt, rec, field)
```
This function returns void pointer to a value. Field can be symbolic constant or
numerical ID. It can be used for both fixed-length and variable-length fields.
(This function is used for fields defined at run-time)

```
char* ur_get_var_as_str(tmplt, rec, field);
```
Function copies data of a variable-length field from UniRec record and append '\0' character.
The function allocates new memory space for the string, it must be freed using free()!
Field can be symbolic constant or numerical ID.

### Set value to UniRec record.
Following functions are used to set a value to specified field in a record.

Parameters:
- "tmplt" - Pointer to UniRec template
- "rec" - Pointer to UniRec record, which is created using given template.
- "field" - Identifier of a field.
- "value" - Value which is copied to the record.

```
ur_set(tmplt, rec, field, value)  // field must be a symbolic constant ...
```
This function assumes value of an appropriate type of a specific field (int, uint, ...).
Because of this, the field must be a symbolic constant (i.e. "F_name") not a numerical ID.
It can be used just for fixed size fields (not for string and bytes).

To set dynamically defined field, use ur_get_ptr_by_id() and write to that pointer.

```
ur_set_var(tmplt, rec, field, val_ptr, val_len)
```
This function is used to set variable-length fields. Field can be symbolic constant or
numerical ID.
For better performance use function ur_clear_varlen, before setting all variable fields in record.

Parameters:
- "val_ptr" - Pointer to value.
- "val_len" - Length of a value. (length which will be copied)

```
 ur_clear_varlen(tmplt, rec);
```
This function will clear all variable-length fields. It can be used for better performance of setting
content to variable-length fields. Use this function before setting of all the variable-length
fields. 
```
ur_set_string(tmplt, rec, field, str) //
```
Set string to the UniRec record. Value is a C-style string, length is determined
automatically by strlen()  ('\0' is not included in the record)

- "str" - Pointer to a string.

### Size of a fixed-length, static field
```
ur_get_size(field) // field must be a symbolic constant ...; for static fields only
```
Returns size of a field. Field has to be statically defined.
For variable-length fields it returns -1. To get size of variable-length field use
function  ur_get_var_len().

### Size of variable-length field
```
ur_get_var_len(tmplt, rec, field)
```
Returns length of a variable-length field. Field can be symbolic constant or
numerical ID.

### Size of a fixed-length part of a record
```
ur_rec_fixlen_size(tmplt)
```
Returns size of a fixed-length part of a record.

### Size of a variable-length part of a record
```
ur_rec_varlen_size(tmplt, rec)
```
Returns size of a variable-length part of a record.

### Size of a record
```
ur_rec_size(tmplt, rec)
```
Returns total size of whole UniRec record.

### Check template's fields
```
ur_is_present(tmplt, field)
```
Returns non-zero if field is present, zero otherwise.

### Check type of a field (variable-length or fixed-length)
```
ur_is_varlen(field)
ur_is_fixlen(field)
```
Returns non-zero if field is dynamic, zero otherwise.

### ID of a field (dynamic or static)
```
ur_field_id_t ur_get_id_by_name(const char *name);
```
Function returns id of a field by name of the field, or UR_E_INVALID_NAME if
the name is not known.

### Create UniRec record
```
void* ur_create_record(const ur_template_t *tmplt, uint16_t max_var_size);
```
Allocates memory for a record with given template. It allocates N+M bytes,
where N is the size of fixed-length part of the record (inferred from template),
and M is the size of variable-length, which must be provided by caller.

Parameters:
- "tmplt" - Pointer to UniRec template.
- "max_var_size" - Size of variable-length part, i.e. sum of lengths of all variable-
length fields. If it is not known at the time of record creation, use
UR_MAX_SIZE, which allocates enough memory to hold the largest possible UniRec
record (65535 bytes). Set to 0 if there are no variable-length fields in the template

### Free UniRec record
```
void ur_free_record(void *record);
```
Free memory allocated for UniRec record. You can call system free() on the
record as well.

### Clone record
```
ur_clone_record(tmplt, src)
```
Function creates new UniRec record and fills it with the data given by parameter.
It returns Pointer to a new UniRec record.

Parameters:
- "tmplt" Pointer to UniRec template
- "src" Pointer to source record


### Copy fields
```
void ur_copy_fields(dst_tmplt, dst, src_tmplt, src);
```

Copies all fields present in both templates from src to dst.

The function compares src_tmplt and dst_tmplt and for each field present in both
templates it sets the value of field in dst to a corresponding value in src.

Parameters:
- "dst_tmplt" - Pointer to destination UniRec template.
- "dst" - Pointer to destination record. It must point to a memory of enough size.
- "src_tmplt" - Pointer to source UniRec template.
- "src" - Pointer to source record.


### Iterate over fields of a template
```
ur_iter_fields(tmplt, id);
```
This function can be used to iterate over all fields of a given template.
It returns ID of the next field present in the template after a given ID.
If ID is set to UR_ITER_BEGIN, it returns the first fields. If no more
fields are present, UR_ITER_END is returned.The order of fields is given
by the order in which they are defined.

The order of fields is given by the order in which they are defined.

Parameters:
- "tmplt" - Pointer to a template to iterate over.
- "id" - Field ID returned in last iteration or UR_ITER_BEGIN to get first value.

Returns ID of the next field or UR_ITER_END if no more fields are present.

Example usage:
```
ur_field_id_t id = UR_ITER_BEGIN;
while ((id = ur_iter_fields(tmplt, id)) != UR_ITER_END) {
...
}
```

```
ur_iter_fields_record_order(tmplt, id);
```

This function can be used to iterate over all fields of a given template.
It returns n-th ID of a record specified by index.
If the return value is UR_ITER_END. The index is higher than count of fields
in the template.

The order of fields is given by the order in the record
Parameters:
- "tmplt" Template to iterate over.
- "id" Field ID returned in last iteration or UR_ITER_BEGIN to
              get first value.
Returns ID of the next field or UR_ITER_END if no more fields are present.

Example usage:
```
int i = 0;
while ((id = ur_iter_fields_record_order(tmplt, i++)) != UR_ITER_END) {
...
}
```
