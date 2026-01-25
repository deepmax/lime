#include "parser.h"
#include "token.h"
#include "lexer.h"
#include "types.h"
#include "panic.h"
#include "context.h"
#include "builtins.h"
#include "ast.h"
#include "vector.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static token_t look;
static context_t* context;

ast_t* factor();
ast_t* expression();
ast_t* ident();
ast_t* func_call(const char* id);
void statements(ast_block_t* block, token_type_t finish);

typedef struct
{
    token_type_t token_type;
    int16_t prec;
} bin_op_prec_t;

typedef struct
{
    char* id;
    type_t type;
} func_param_t;

const bin_op_prec_t BIN_OP_PREC[] = {
    {TK_MUL, 90},
    {TK_DIV, 90},
    {TK_MOD, 90},
    {TK_PLUS, 80},
    {TK_MINUS, 80},
    {TK_LT, 70},
    {TK_LTE, 70},
    {TK_GT, 70},
    {TK_GTE, 70},
    {TK_EQ, 60},
    {TK_NE, 60},
    {TK_AND_BIT, 55},
    {TK_XOR_BIT, 54},
    {TK_OR_BIT, 53},
    {TK_AND, 50},
    {TK_OR, 40}
};

static const token_type_t UNARY[] = {
    TK_PLUS,
    TK_MINUS,
    TK_NOT,
};

int16_t token_prec(token_type_t token_type)
{
    for (int i = 0; i<sizeof (BIN_OP_PREC) / sizeof (BIN_OP_PREC[0]); i++)
    {
        if (BIN_OP_PREC[i].token_type == token_type)
            return BIN_OP_PREC[i].prec;
    }
    return -1;
}

bool_t is_binary(token_type_t token_type)
{
    return token_prec(token_type) > 0;
}

bool_t is_unary(token_type_t token_type)
{
    for (int i = 0; i<sizeof (UNARY) / sizeof (UNARY[0]); i++)
    {
        if (UNARY[i] == token_type)
            return true;
    }
    return false;
}

bool_t is_logical(token_type_t token_type)
{
    switch (token_type)
    {
        case TK_NOT:
        case TK_EQ:
        case TK_NE:
        case TK_AND:
        case TK_OR:
        case TK_LT:
        case TK_GT:
        case TK_LTE:
        case TK_GTE:
            return true;
        default:
            return false;
    }
}

void match(token_type_t token_type)
{
    if (look.type == token_type)
        look = lexer_next();
    else
        panic("Not expected token");
}

char* peek_ident()
{
    if (look.type != TK_IDENT)
        panic("An identifier is expected");

    return look.value.as_str;
}

type_t peek_data_type()
{
    for (int i = 0; i < sizeof (BUILTIN_DATATYPES) / sizeof (BUILTIN_DATATYPES[0]); i++)
    {
        if (strcmp(BUILTIN_DATATYPES[i].name, look.value.as_str) == 0)
            return BUILTIN_DATATYPES[i].type;
    }

    if (look.type != TK_IDENT)
        panic("A data type is expected");

    panic("Uknown data type.");
}

type_t data_type()
{
    type_t t = peek_data_type();

    if (t != MT_UNKNOWN)
    {
        match(look.type);
        return t;
    }

    panic("Uknown data type.");

    return MT_UNKNOWN;
}

type_t infer_binary_expr_type(token_type_t op, type_t lhs_type, type_t rhs_type)
{
    type_t infered_type = MT_UNKNOWN;

    if ((is_integer_type(lhs_type) && is_integer_type(rhs_type)) ||
        (is_integer_type(lhs_type) && rhs_type == MT_REAL) ||
        (is_integer_type(rhs_type) && lhs_type == MT_REAL) ||
        (lhs_type == MT_REAL && rhs_type == MT_REAL) ||
        (lhs_type == MT_BOOL && rhs_type == MT_BOOL))
    {
        if (is_logical(op))
        {
            infered_type = MT_BOOL;
        }
        else
        {
            infered_type = mix_numerical_types(lhs_type, rhs_type);
        }
    }

    return infered_type;
}

type_t infer_unary_expr_type(token_type_t op, type_t type)
{
    if (is_logical(op))
    {
        return MT_BOOL;
    }

    type_t infered_type = MT_UNKNOWN;

    if (is_integer_type(type) || is_bool_type(type))
    {
        infered_type = type;
    } else if (type == MT_REAL)
    {
        infered_type = type;
    }

    return infered_type;
}

ast_t* binary_expr(int16_t min_prec, ast_t* lhs)
{
    int32_t tok_prec;

    while (true)
    {
        tok_prec = token_prec(look.type);
        if (tok_prec < min_prec)
            break;

        token_type_t op = look.type;

        match(op);

        ast_t* rhs = factor();

        int16_t next_prec = token_prec(look.type);

        if (tok_prec < next_prec)
        {
            rhs = binary_expr(tok_prec + 1, rhs);
        }

        type_t mixed_type = infer_binary_expr_type(op, lhs->base->type, rhs->base->type);

        if (mixed_type == MT_UNKNOWN)
        {
            panic("Type unknown or mismatch for binary expression!");
        }

        lhs = (ast_t*) ast_new_binary(mixed_type, op, lhs, rhs);
    }

    return lhs;
}

ast_t* unary_expr()
{
    token_type_t unary = look.type;
    match(unary);

    ast_t* expr = factor();

    type_t type = infer_unary_expr_type(unary, expr->base->type);

    if (type == MT_UNKNOWN)
    {
        panic("Type unknown or mismatch for unary expression!");
    }

    return (ast_t*) ast_new_unary(type, unary, expr);
}

ast_t* expression()
{
    return binary_expr(0, factor());
}

ast_t* factor()
{
    ast_t* node = NULL;

    if (look.type == TK_IDENT)
    {
        node = ident();
    } 
    else if (look.type == TK_L_PAREN)
    {
        match(TK_L_PAREN);
        node = expression();
        match(TK_R_PAREN);
    }
    else if (look.type == TK_TRUE)
    {
        match(TK_TRUE);
        value_t value;
        value.as_int64 = 1;
        node = (ast_t*) ast_new_constant(MT_BOOL, value);
    }
    else if (look.type == TK_FALSE)
    {
        match(TK_FALSE);
        value_t value;
        value.as_int64 = 0;
        node = (ast_t*) ast_new_constant(MT_BOOL, value);
    }
    else if (look.type == TK_INT8)
    {
        value_t value;
        value.as_int64 = (int8_t)look.value.as_int64;
        match(TK_INT8);
        node = (ast_t*) ast_new_constant(MT_INT8, value);
    }
    else if (look.type == TK_INT16)
    {
        value_t value;
        value.as_int64 = (int16_t)look.value.as_int64;
        match(TK_INT16);
        node = (ast_t*) ast_new_constant(MT_INT16, value);
    }
    else if (look.type == TK_INT32)
    {
        value_t value;
        value.as_int64 = (int32_t)look.value.as_int64;
        match(TK_INT32);
        node = (ast_t*) ast_new_constant(MT_INT32, value);
    }
    else if (look.type == TK_INT64)
    {
        value_t value;
        value.as_int64 = look.value.as_int64;
        match(TK_INT64);
        node = (ast_t*) ast_new_constant(MT_INT64, value);
    }
    else if (look.type == TK_UINT8)
    {
        value_t value;
        value.as_int64 = (uint8_t)look.value.as_int64;
        match(TK_UINT8);
        node = (ast_t*) ast_new_constant(MT_UINT8, value);
    }
    else if (look.type == TK_UINT16)
    {
        value_t value;
        value.as_int64 = (uint16_t)look.value.as_int64;
        match(TK_UINT16);
        node = (ast_t*) ast_new_constant(MT_UINT16, value);
    }
    else if (look.type == TK_UINT32)
    {
        value_t value;
        value.as_int64 = (uint32_t)look.value.as_int64;
        match(TK_UINT32);
        node = (ast_t*) ast_new_constant(MT_UINT32, value);
    }
    else if (look.type == TK_UINT64)
    {
        value_t value;
        value.as_uint64 = (uint64_t)look.value.as_int64;
        match(TK_UINT64);
        node = (ast_t*) ast_new_constant(MT_UINT64, value);
    }
    else if (look.type == TK_REAL)
    {
        value_t value;
        value.as_real = look.value.as_real;
        match(TK_REAL);
        node = (ast_t*) ast_new_constant(MT_REAL, value);
    }
    else if (look.type == TK_STR)
    {
        value_t value;
        value.as_str = look.value.as_str;
        match(TK_STR);
        node = (ast_t*) ast_new_constant(MT_STR, value);
    }
    else if (is_unary(look.type))
    {
        node = unary_expr();
    }
    else
    {
        panic("Unknown factor!");
    }

    return node;
}

ast_t* assign(bool_t new_variable, const char* id)
{
    symbol_t* s = context_get(context, id, false);

    if (s == NULL)
    {
        panic("Identifier is not defined.");
    }

    match(TK_ASSIGN);

    ast_t* expr = expression();
    type_t expr_type = expr->base->type;

    if (expr_type == MT_UNKNOWN)
    {
        panic("No type to assign.");
    }

    if (s->type == MT_UNKNOWN)
    {
        s->type = expr_type;
    }
    else if (s->type != expr_type)
    {
        if (can_implicitly_cast_integer(expr_type, s->type))
        {

        }
        else
        {
            panic("Assignment type mismatch.");
        }
    }

    return (ast_t*) ast_new_assign(MT_UNKNOWN, s, expr, new_variable);
}

ast_t* var()
{
    match(TK_VAR);

    const char* id = peek_ident();

    match(TK_IDENT);

    if (context_get(context, id, true) != NULL)
        panic("Identifier is already defined.");

    symbol_t* s = context_add(context, id, MT_UNKNOWN);

    if (look.type == TK_COLON)
    {
        match(TK_COLON);
        s->type = data_type();
    }

    if (look.type == TK_ASSIGN)
        return assign(true, id);

    if (s->type == MT_UNKNOWN)
        panic("No type declared for the variable.");

    return NULL;
}

ast_t* ident()
{
    const char* id = peek_ident();

    match(TK_IDENT);

    if (look.type == TK_L_PAREN)
        return func_call(id);

    symbol_t* s = context_get(context, id, false) ;
    if (s == NULL)
        panic("Identifier is not defined.");

    if (look.type == TK_ASSIGN)
        return assign(false, id);

    return (ast_t*) ast_new_variable(s->type, context_get(context, id, false));
}

ast_t* block(block_t type, vector_t* params)
{
    match(TK_L_BRACE);

    context_t* new_context = context_new(context, type);
    
    if (params != NULL)
    {
        for (size_t i = 0; i < vec_size(params); i++)
        {
            func_param_t* param = vec_get(params, i);
            context_add(new_context, param->id, param->type);
        }
    }

    ast_block_t* blck = ast_new_block(MT_UNKNOWN, new_context);

    context = new_context;

    statements(blck, TK_R_BRACE);

    match(TK_R_BRACE);

    context = new_context->parent;

    return (ast_t*) blck;
}

ast_t* if_cond()
{
    match(TK_IF);
    ast_t* condition = expression();
    ast_t* if_then = block(MB_NORMAL, NULL);
    ast_t* if_else = NULL;
    if (look.type == TK_ELSE)
    {
        match(TK_ELSE);
        if (look.type == TK_IF)
            if_else = if_cond();
        else
            if_else = block(MB_NORMAL, NULL);
    }
    return (ast_t*) ast_new_if_cond(MT_UNKNOWN, condition, if_then, if_else);
}


ast_t* func_decl()
{
    match(TK_FUNC);

    const char* id = peek_ident();
    match(TK_IDENT);

    if (context_get(context, id, false) != NULL)
        panic("Identifier is already defined.");

    symbol_t* s = context_add(context, id, MT_FUNC);

    match(TK_L_PAREN);

    vector_t* params = vec_new(0);

    while (look.type != TK_R_PAREN)
    {
        func_param_t* param = malloc(sizeof(func_param_t));
        param->id = peek_ident();
        match(TK_IDENT);
        match(TK_COLON);
        param->type = data_type();

        vec_append(params, param);

        if (look.type == TK_R_PAREN)
            break;
        match(TK_COMMA);
    }
    match(TK_R_PAREN);

    match(TK_COLON);
    type_t ret_type = data_type();

    s->type = MT_FUNC;
    s->extra.func.ret_type = ret_type;
    
    s->extra.func.param_types = vec_new(0);
    for (size_t i = 0; i < vec_size(params); i++)
    {
        func_param_t* param = vec_get(params, i);
        type_t* param_type = malloc(sizeof(type_t));
        *param_type = param->type;
        vec_append(s->extra.func.param_types, param_type);
    }

    return (ast_t*) ast_new_func_decl(ret_type, s, (ast_block_t*) block(MB_FUNC, params), vec_size(params));
}

ast_t* func_ret()
{
    if (context_get_func(context) == NULL)
        panic("Return statement outside of function.");

    match(TK_RETURN);
    return (ast_t*) ast_new_func_return(MT_UNKNOWN, expression());
}

ast_t* builting_func_call(const builtin_func_t* builtin)
{
    match(TK_L_PAREN);

    vector_t* args = vec_new(0);

    while (look.type != TK_R_PAREN)
    {
        vec_append(args, expression());
        if (look.type == TK_R_PAREN)
            break;
        match(TK_COMMA);
    }
    match(TK_R_PAREN);
    

                // // Check if the argument type is acceptable
            // if (!is_type_acceptable(arg_type, builtin->acceptable_types))
            // {
            //     panic("Builtin function argument type mismatch.");
            // }



    // Validate argument count (255 means variadic, skip check)
    if (builtin->arg_count != 255 && vec_size(args) != builtin->arg_count)
    {
        panic("Builtin function argument count mismatch.");
    }

    return (ast_t*) ast_new_builtin_call(builtin->ret_type, builtin->name, args);
}

ast_t* func_call(const char* id)
{
    const builtin_func_t* builtin = builtin_lookup(id);

    if (builtin != NULL)
    {
        return builting_func_call(builtin);
    }

    symbol_t* s = context_get(context, id, false);

    if (s == NULL)
        panic("Identifier is not defined.");

    match(TK_L_PAREN);

    vector_t* args = vec_new(0);
    while (look.type != TK_R_PAREN)
    {
        ast_t* expr = expression();
        vec_append(args, expr);
        if (look.type == TK_R_PAREN)
            break;
        match(TK_COMMA);
    }
    match(TK_R_PAREN);


    vector_t* param_types = s->extra.func.param_types;
    size_t param_count = vec_size(param_types);

    if (param_count != vec_size(args))
    {
        panic("Function paramters passed count mismatch.");
    }

    for (size_t i = 0; i < param_count; i++)
    {
        type_t arg_type = ((ast_t*)vec_get(args, i))->base->type;
        type_t param_type = *((type_t*)vec_get(param_types, i++));

        if (arg_type == MT_UNKNOWN)
        {
            panic("No type to pass as parameter.");
        }

        if (arg_type != param_type)
        {
            if (can_implicitly_cast_integer(arg_type, param_type))
            {
            }
            else
            {
                panic("Function parameter type mismatch.");
            }
        }
    }

    return (ast_t*) ast_new_func_call(s->extra.func.ret_type, s, args);
}

ast_t* for_loop()
{
    context_t* new_context = context_new(context, MB_NORMAL);
    context = new_context;

    match(TK_FOR);
    ast_t* init = look.type == TK_VAR? var() : expression();
    match(TK_SEMICOLON);
    ast_t* condition = expression();
    match(TK_SEMICOLON);
    ast_t* post = expression();

    ast_block_t* for_block = (ast_block_t*) ast_new_for_loop(MT_UNKNOWN, init, condition, post, block(MB_LOOP, NULL));

    context = new_context->parent;

    return (ast_t*) for_block;
}

ast_t* break_loop()
{
    if (context_get_loop(context) == NULL)
        panic("Break statement outside of loop.");
    match(TK_BREAK);
    return (ast_t*) ast_new_break_loop(MT_UNKNOWN, context_get_loop(context));
}

ast_t* continue_loop()
{
    if (context_get_loop(context) == NULL)
        panic("Continue statement outside of loop.");
    match(TK_CONTINUE);
    return (ast_t*) ast_new_continue_loop(MT_UNKNOWN, context_get_loop(context));
}

ast_t* semicolon()
{
    match(TK_SEMICOLON);
    return NULL;
}

ast_t* statement(bool_t* drop)
{
    switch (look.type)
    {
    case TK_SEMICOLON:
        return semicolon();
    case TK_VAR:
        return var();
    case TK_IF:
        return if_cond();
    case TK_FOR:
        return for_loop();
    case TK_BREAK:
        return break_loop();
    case TK_CONTINUE:
        return continue_loop();
    case TK_FUNC:
        return func_decl();
    case TK_RETURN:
        return func_ret();
    case TK_L_BRACE:
        return block(MB_NORMAL, NULL);
    default:
        if(drop) *drop = true;
        return expression();
    }
    return NULL;
}

void statements(ast_block_t* block, token_type_t finish)
{
    while (look.type != finish)
    {
        bool_t drop = false;
        vec_append(block->nodes, statement(&drop));

        if (drop == true)
            vec_append(block->nodes, (ast_t*)ast_new_single_opcode(MT_UNKNOWN, DROP));
    }
}

void parser_init()
{
    look.type = TK_BAD;
    look.col = 0;
    look.row = 0;
    look = lexer_next();
    context = global_context;
}

void parser_load_file(const char* filename)
{
    lexer_load_file(filename);
    parser_init();
}

void parser_load_stdin()
{
    lexer_load_stdin();
    parser_init();
}

void parser_free()
{
    lexer_free();
}

void parser_parse()
{
    vm_init();

    ast_block_t* block = ast_new_block(MT_UNKNOWN, global_context);

    statements(block, TK_FIN);

    eval((ast_t*) block);

    EMIT(HALT);

    vm_dasm("out.asm");
    
    vm_exec();
}
