#ifndef BUILTINS_H
#define BUILTINS_H

#include "types.h"
#include "token.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    const char* name;
    uint8_t arg_count;
    type_t ret_type;
    uint8_t opcode;  // VM opcode to emit, or 0 if not applicable
    const type_t* acceptable_types;  // Array of acceptable argument types, terminated by MT_UNKNOWN
} builtin_func_t;

typedef struct
{
    token_type_t token;
    type_t type;
    const char* name;
} builtin_datatype_t;

static const builtin_datatype_t BUILTIN_DATATYPES[] = {
    {TK_INT8_T, MT_INT8, "i8"},
    {TK_INT16_T, MT_INT16, "i16"},
    {TK_INT32_T, MT_INT32, "i32"},
    {TK_INT64_T, MT_INT64, "i64"},
    {TK_INT8_T, MT_UINT8, "u8"},
    {TK_UINT16_T, MT_UINT16, "u16"},
    {TK_UINT32_T, MT_UINT32, "u32"},
    {TK_UINT64_T, MT_UINT64, "u64"},
    {TK_STR_T, MT_STR, "str"},
    {TK_REAL_T, MT_REAL, "real"},
    {TK_BOOL_T, MT_BOOL, "bool"},
    {TK_VOID_T, MT_VOID, "void"},
    {TK_ARRAY_T, MT_ARRAY, "array"},
};

const builtin_func_t* builtin_lookup(const char* name);
bool_t builtin_is_reserved(const char* name);
bool_t is_builtin_type_acceptable(type_t type, const type_t* acceptable_types);

#ifdef __cplusplus
}
#endif


#endif /* BUILTINS_H */
