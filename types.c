#include "types.h"

uint8_t msb(uint64_t x)
{
    uint8_t pos = 0;

    if (x >> 16) { pos += 16; x >>= 16; }
    if (x >> 8 ) { pos += 8;  x >>= 8;  }
    if (x >> 4 ) { pos += 4;  x >>= 4;  }
    if (x >> 2 ) { pos += 2;  x >>= 2;  }
    if (x >> 1 ) { pos += 1;            }

    return pos;
}

size_t min_coverage_size(size_t x)
{
    return 1 << (msb(x) + 1);
}

bool_t is_integer_type(type_t type)
{
    return type == MT_INT8 || type == MT_INT16 || type == MT_INT32 || type == MT_INT64 ||
        type == MT_UINT8 || type == MT_UINT16 || type == MT_UINT32 || type == MT_UINT64;
}

bool_t is_str_type(type_t type)
{
    return type == MT_STR;
}

bool_t is_real_type(type_t type)
{
    return type == MT_REAL;
}

bool_t is_bool_type(type_t type)
{
    return type == MT_BOOL;
}

bool_t is_unsigned_integer_type(type_t type)
{
    return type == MT_UINT8 || type == MT_UINT16 || type == MT_UINT32 || type == MT_UINT64;
}

size_t type_size(type_t type)
{
    switch (type)
    {
        case MT_BOOL:
        case MT_UINT8:
        case MT_INT8:
            return 1;
        case MT_UINT16:
        case MT_INT16:
            return 2;
        case MT_UINT32:
        case MT_INT32:
            return 4;
        case MT_UINT64:
        case MT_INT64:
        case MT_REAL:
            return 8;
        case MT_STR:
        case MT_ARRAY:
        case MT_FUNC:
            return 16;
        default:
            return 0;
    }
}

// Helper function to check if an integer type can be implicitly cast to another
// Allows casting from smaller integer types to larger ones
bool_t can_implicitly_cast_integer(type_t from, type_t to)
{
    // Both must be integer types
    if (!is_integer_type(from) || !is_integer_type(to))
        return false;

    // Same type - no cast needed
    if (from == to)
        return true;

    // Allow casting from smaller to larger (widening)
    return type_size(from) <= type_size(to);
}

bool_t need_explicit_cast_integer(type_t from, type_t to)
{
    // Both must be integer types
    if (!is_integer_type(from) || !is_integer_type(to))
        return false;

    // Same type - no cast needed
    if (from == to)
        return false;

    return type_size(from) >= type_size(to);
}

type_t mix_numerical_types(type_t t1, type_t t2)
{
    bool_t t1int = is_integer_type(t1);
    bool_t t2int = is_integer_type(t2);

    if (t1int && t2int)
    {
        if (type_size(t1) >= type_size(t2))
            return t1;
        else
            return t2;
    }

    if ((t1int && t2 == MT_REAL) ||
        (t2int && t1 == MT_REAL) ||
        (t1 == MT_REAL && t2 == MT_REAL))
    {
        return MT_REAL;
    }

    return MT_UNKNOWN;
}
