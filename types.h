#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef double real_t;
typedef bool bool_t;
typedef char char_t;

typedef enum
{
    MT_UNKNOWN,
    MT_INT8,
    MT_INT16,
    MT_INT32,
    MT_INT64,
    MT_UINT8,
    MT_UINT16,
    MT_UINT32,
    MT_UINT64,
    MT_VOID,
    MT_BOOL,
    MT_STR,
    MT_REAL,
    MT_FUNC,
    MT_ARRAY,
} type_t;

typedef union
{
    char_t* as_str;
    bool_t as_bool;
    real_t as_real;
    int8_t as_int8;
    int16_t as_int16;
    int32_t as_int32;
    int64_t as_int64;
    uint8_t as_uint8;
    uint16_t as_uint16;
    uint32_t as_uint32;
    uint64_t as_uint64;
    uintptr_t as_ptr;
} value_t;

typedef enum
{
    MB_NORMAL,
    MB_LOOP,
    MB_FUNC,
    MB_CLASS,
    MB_GLOBAL,
} block_t;

uint8_t msb(uint64_t x);
size_t min_coverage_size(size_t x);
bool_t is_integer_type(type_t type);
bool_t is_str_type(type_t type);
bool_t is_real_type(type_t type);
bool_t is_bool_type(type_t type);
bool_t is_unsigned_integer_type(type_t type);
size_t type_size(type_t type);
bool_t can_implicitly_cast_integer(type_t from, type_t to);
bool_t need_explicit_cast_integer(type_t from, type_t to);
type_t mix_numerical_types(type_t t1, type_t t2);


#ifdef __cplusplus
}
#endif

#endif /* TYPES_H */
