#include "builtins.h"
#include "types.h"
#include "vm.h"
#include <string.h>

// Acceptable type arrays for builtin functions
static const type_t INTEGER_TYPES[] = {MT_INT8, MT_INT16, MT_INT32, MT_INT64, MT_UINT8, MT_UINT16, MT_UINT32, MT_UINT64, MT_BOOL, MT_UNKNOWN};
static const type_t REAL_TYPES[] = {MT_REAL, MT_UNKNOWN};
static const type_t STR_TYPES[] = {MT_STR, MT_UNKNOWN};
static const type_t NUMERIC_TYPES[] = {MT_INT8, MT_INT16, MT_INT32, MT_INT64, MT_UINT8, MT_UINT16, MT_UINT32, MT_UINT64, MT_REAL, MT_BOOL, MT_UNKNOWN};
static const type_t PRINT_TYPES[] = {MT_INT8, MT_INT16, MT_INT32, MT_INT64, MT_UINT8, MT_UINT16, MT_UINT32, MT_UINT64, MT_REAL, MT_BOOL, MT_STR, MT_UNKNOWN};

static const builtin_func_t BUILTIN_FUNCTIONS[] = {
    {"print", 255, MT_VOID, 0, PRINT_TYPES},  // arg_count 255 means variadic, opcode 0 means dispatch based on type
    {"abs", 1, MT_UNKNOWN, 0, NUMERIC_TYPES},  // opcode 0 means dispatch based on type
    {"mod", 2, MT_REAL, RMOD, REAL_TYPES},
    {"pow", 2, MT_REAL, RPOW, REAL_TYPES},
    {"sqrt", 1, MT_REAL, RSQRT, REAL_TYPES},
    {"exp", 1, MT_REAL, REXP, REAL_TYPES},
    {"sin", 1, MT_REAL, RSIN, REAL_TYPES},
    {"cos", 1, MT_REAL, RCOS, REAL_TYPES},
    {"tan", 1, MT_REAL, RTAN, REAL_TYPES},
    {"acos", 1, MT_REAL, RACOS, REAL_TYPES},
    {"atan2", 2, MT_REAL, RATAN2, REAL_TYPES},
    {"log", 1, MT_REAL, RLOG, REAL_TYPES},
    {"log10", 1, MT_REAL, RLOG10, REAL_TYPES},
    {"log2", 1, MT_REAL, RLOG2, REAL_TYPES},
    {"ceil", 1, MT_REAL, RCEIL, REAL_TYPES},
    {"floor", 1, MT_REAL, RFLOOR, REAL_TYPES},
    {"round", 1, MT_REAL, RROUND, REAL_TYPES},
    {"i8", 1, MT_INT8, I8CAST, INTEGER_TYPES},
    {"u8", 1, MT_UINT8, IU8CAST, INTEGER_TYPES},
    {"i16", 1, MT_INT16, I16CAST, INTEGER_TYPES},
    {"u16", 1, MT_UINT16, IU16CAST, INTEGER_TYPES},
    {"i32", 1, MT_INT32, I32CAST, INTEGER_TYPES},
    {"u32", 1, MT_UINT32, IU32CAST, INTEGER_TYPES},
    {"i64", 1, MT_INT64, I64CAST, INTEGER_TYPES},
    {"u64", 1, MT_UINT64, IU64CAST, INTEGER_TYPES},
    {"itor", 1, MT_REAL, ITOR, INTEGER_TYPES},
    {"rtoi", 1, MT_INT64, RTOI, REAL_TYPES},

};

// TODO: inc and dec for integer and real types need passing address of the variable to the builtin function
// so that they so a inplace operation on the variable

// TODO: make a general len instead of slen ...


#define BUILTIN_COUNT (sizeof(BUILTIN_FUNCTIONS) / sizeof(BUILTIN_FUNCTIONS[0]))
#define BUILTIN_CONSTANT_COUNT (sizeof(BUILTIN_CONSTANTS) / sizeof(BUILTIN_CONSTANTS[0]))

const builtin_func_t* builtin_lookup(const char* name)
{
    for (size_t i = 0; i < BUILTIN_COUNT; i++)
    {
        if (strcmp(BUILTIN_FUNCTIONS[i].name, name) == 0)
        {
            return &BUILTIN_FUNCTIONS[i];
        }
    }
    return NULL;
}

bool_t builtin_is_reserved(const char* name)
{
    return builtin_lookup(name) != NULL;
}

bool is_builtin_type_acceptable(type_t type, const type_t* acceptable_types)
{
    if (acceptable_types == NULL)
        return true;
    
    for (size_t i = 0; acceptable_types[i] != MT_UNKNOWN; i++)
    {
        if (acceptable_types[i] == type)
            return true;
    }
    return false;
}
