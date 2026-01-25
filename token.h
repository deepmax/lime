#ifndef TOKEN_H
#define TOKEN_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    TK_FIN,
    TK_BAD,
    TK_VAR,
    TK_LET,
    TK_FUNC,
    TK_TRUE,
    TK_FALSE,
    TK_IF,
    TK_OF,
    TK_ELSE,
    TK_LOOP,
    TK_FOR,
    TK_IN,
    TK_DOTDOT,
    TK_BREAK,
    TK_CONTINUE,
    TK_RETURN,
    TK_PRINT,
    TK_READ,
    TK_EXIT,
    TK_EXTERN,
    TK_STRUCT,
    TK_COMMA,
    TK_COLON,
    TK_SEMICOLON,
    TK_PERIOD,
    TK_BACKSLASH,
    TK_ASSIGN,
    TK_PLUS,
    TK_MINUS,
    TK_DIV,
    TK_MUL,
    TK_MOD,
    TK_OR_BIT,
    TK_AND_BIT,
    TK_XOR_BIT,
    TK_OR,
    TK_AND,
    TK_NOT,
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_GT,
    TK_LTE,
    TK_GTE,
    TK_QUESTION,
    TK_L_BRACE,
    TK_R_BRACE,
    TK_L_PAREN,
    TK_R_PAREN,
    TK_L_BRACKET,
    TK_R_BRACKET,
    TK_REAL,
    TK_BOOL,
    TK_VOID,
    TK_STR,
    TK_INT8,
    TK_INT16,
    TK_INT32,
    TK_INT64,
    TK_UINT8,
    TK_UINT16,
    TK_UINT32,
    TK_UINT64,
    TK_REAL_T,
    TK_BOOL_T,
    TK_VOID_T,
    TK_STR_T,
    TK_INT8_T,
    TK_INT16_T,
    TK_INT32_T,
    TK_INT64_T,
    TK_UINT8_T,
    TK_UINT16_T,
    TK_UINT32_T,
    TK_UINT64_T,
    TK_ARRAY_T,
    TK_IDENT,
    TK_LAST_TOKEN,
} token_type_t;

typedef value_t token_value_t;

typedef struct
{
    token_type_t type;
    token_value_t value;
    uint32_t row;
    uint32_t col;
} token_t;


#ifdef __cplusplus
}
#endif

#endif /* TOKEN_H */
