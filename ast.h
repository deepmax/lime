#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "types.h"
#include "vector.h"
#include "token.h"
#include "context.h"
#include "vm.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void(*eval_t)(void*);

struct ast_t
{
    struct ast_t* base;
    eval_t eval;
    type_t type;
};

typedef struct ast_t ast_t;

typedef struct
{
    ast_t* base;
    value_t value;
} ast_constant_t;

typedef struct
{
    ast_t* base;
    ast_t* expr;
    token_type_t op;
} ast_unary_t;

typedef struct
{
    ast_t* base;
    ast_t* lhs_expr;
    ast_t* rhs_expr;
    token_type_t op;
} ast_binary_t;

typedef struct
{
    ast_t* base;
    context_t* context;
    vector_t* nodes;
} ast_block_t;

typedef struct
{
    ast_t* base;
    uint8_t opcode;
} ast_single_opcode_t;

typedef struct
{
    ast_t* base;
    ast_t* condition;
    ast_t* if_then;
    ast_t* if_else;
} ast_if_cond_t;

typedef struct
{
    ast_t* base;
    ast_t* expr;
    symbol_t* symbol;
    bool_t new_variable;
} ast_assign_t;

typedef struct
{
    ast_t* base;
    symbol_t* symbol;
} ast_variable_t;

typedef struct
{
    ast_t* base;
    symbol_t* symbol;
    ast_block_t* body;
    uint16_t args;
    type_t ret_type;
} ast_func_decl_t;

typedef struct
{
    ast_t* base;
    symbol_t* symbol;
    vector_t* args;
} ast_func_call_t;

typedef struct
{
    ast_t* base;
    const char* name;
    vector_t* args;
    type_t ret_type;
} ast_builtin_call_t;

typedef struct
{
    ast_t* base;
    ast_t* expr;
} ast_func_return_t;

typedef struct
{
    ast_t* base;
    ast_t* init;
    ast_t* condition;
    ast_t* post;
    ast_t* body;
    loop_t* loop;

} ast_for_loop_t;

typedef struct
{
    ast_t* base;
    loop_t* loop;
} ast_break_loop_t;

typedef struct
{
    ast_t* base;
    loop_t* loop;
} ast_continue_loop_t;

void eval(ast_t* ast);
ast_constant_t* ast_new_constant(type_t type, value_t value);
ast_unary_t* ast_new_unary(type_t type, token_type_t op, ast_t* expr);
ast_binary_t* ast_new_binary(type_t type, token_type_t op, ast_t* lhs_expr, ast_t* rhs_expr);
ast_block_t* ast_new_block(type_t type, context_t* context);
ast_single_opcode_t* ast_new_single_opcode(type_t type, uint8_t opcode);
ast_if_cond_t* ast_new_if_cond(type_t type, ast_t* condition, ast_t* if_then, ast_t* if_else);
ast_assign_t* ast_new_assign(type_t type, symbol_t* symbol, ast_t* expr, bool_t new_variable);
ast_variable_t* ast_new_variable(type_t type, symbol_t* symbol);
ast_func_decl_t* ast_new_func_decl(type_t type, symbol_t* symbol, ast_block_t* body, uint16_t args);
ast_func_call_t* ast_new_func_call(type_t type, symbol_t* symbol, vector_t* args);
ast_builtin_call_t* ast_new_builtin_call(type_t type, const char* name, vector_t* args);
ast_func_return_t* ast_new_func_return(type_t type, ast_t* expr);
ast_for_loop_t* ast_new_for_loop(type_t type, ast_t* init, ast_t* condition, ast_t* post, ast_t* body);
ast_break_loop_t* ast_new_break_loop(type_t type, loop_t* loop);
ast_continue_loop_t* ast_new_continue_loop(type_t type, loop_t* loop);

#ifdef __cplusplus
}
#endif

#endif /* AST_H */
