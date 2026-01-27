#include "ast.h"
#include "builtins.h"
#include "panic.h"
#include "token.h"
#include "vm.h"
#include "jump.h"
#include "utf8.h"
#include "types.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

void eval(ast_t* ast)
{
    if (ast)
        ast->base->eval(ast);
}

void eval_constant(ast_constant_t* ast)
{
    type_t type = ast->base->type;
    value_t value = ast->value;

    if (is_integer_type(type) || is_bool_type(type))
    {
        if (value.as_int64 == 0) {
            EMIT(ICONST_0);
        } else if (value.as_int64 == 1) {
            EMIT(ICONST_1);
        } else {
            switch(type)
            {
                case MT_INT8:
                case MT_UINT8:
                    EMIT(I8CONST, NUM8(value.as_int8));
                    break;
                case MT_INT16:
                case MT_UINT16:
                    EMIT(I16CONST, NUM16(value.as_int16));
                    break;
                case MT_INT32:
                case MT_UINT32:
                    EMIT(I32CONST, NUM32(value.as_int32));
                    break;
                case MT_INT64:
                case MT_UINT64:
                    EMIT(I64CONST, NUM64(value.as_int64));
                    break;
                default:
                    ;
            }
        }
    }
    else if (is_real_type(type))
    {
        if (ast->value.as_real == 0.0)
        {
            EMIT(RCONST_0);
        }
        else if (ast->value.as_real == 1.0)
        {
            EMIT(RCONST_1);
        }
        else
        {
            EMIT(RCONST, NUM64(ast->value.as_uint64));
        }
    }
    else if (is_str_type(type))
    {
        uint16_t addr = vm_data_used();
        vm_data_emit((uint8_t*)ast->value.as_str, utf8size((utf8_int8_t*)ast->value.as_str));
        EMIT(XCONST, NUM16(addr));
    }
}

void eval_binary(ast_binary_t* ast)
{
    type_t lhs_type = ast->lhs_expr->base->type;
    type_t rhs_type = ast->rhs_expr->base->type;
    
    eval(ast->lhs_expr);

    if (is_integer_type(lhs_type) && is_real_type(rhs_type))
    {
        EMIT(ITOR);
        lhs_type = MT_REAL;
    }

    JUMP_NEW(short_circuit);

    if (ast->op == TK_AND || ast->op == TK_OR)
    {
        EMIT(DUP);
        if (ast->op == TK_AND)
            JUMP(JEZ, short_circuit);
        else if (ast->op == TK_OR)
            JUMP(JNZ, short_circuit);
    }

    eval(ast->rhs_expr);

    if (is_integer_type(rhs_type) && is_real_type(lhs_type))
    {
        EMIT(ITOR);
        rhs_type = MT_REAL;
    }

    if ((is_integer_type(lhs_type) && is_integer_type(rhs_type)) ||
        (is_bool_type(lhs_type) && is_bool_type(rhs_type)))
    {
        switch (ast->op)
        {
        case TK_PLUS:
            EMIT(IADD);
            break;
        case TK_MINUS:
            EMIT(ISUB);
            break;
        case TK_MUL:
            EMIT(IMUL);
            break;
        case TK_DIV:
            EMIT(IDIV);
            break;
        case TK_MOD:
            EMIT(IMOD);
            break;
        case TK_EQ:
            EMIT(IEQ);
            break;
        case TK_NE:
            EMIT(INQ);
            break;
        case TK_LT:
            EMIT(ILT);
            break;
        case TK_LTE:
            EMIT(ILE);
            break;
        case TK_GT:
            EMIT(IGT);
            break;
        case TK_GTE:
            EMIT(IGE);
            break;
        case TK_AND_BIT:
            EMIT(IBAND);
            break;
        case TK_OR_BIT:
            EMIT(IBOR);
            break;
        case TK_XOR_BIT:
            EMIT(IBXOR);
            break;
        case TK_AND:
            EMIT(IAND);
            break;
        case TK_OR:
            EMIT(IOR);
            break;
        // Note: Shift operators (ISHL, ISHR) exist in VM but no token types yet
        default:
            panic("Unknown integer binary operation.");
        }
    }
    else if (is_real_type(lhs_type) || is_real_type(rhs_type)) // Is that enough ?
    {
        switch (ast->op)
        {
        case TK_PLUS:
            EMIT(RADD);
            break;
        case TK_MINUS:
            EMIT(RSUB);
            break;
        case TK_MUL:
            EMIT(RMUL);
            break;
        case TK_DIV:
            EMIT(RDIV);
            break;
        case TK_MOD:
            EMIT(RMOD);
            break;
        case TK_EQ:
            EMIT(REQ);
            break;
        case TK_NE:
            EMIT(RNQ);
            break;
        case TK_LT:
            EMIT(RLT);
            break;
        case TK_LTE:
            EMIT(RLE);
            break;
        case TK_GT:
            EMIT(RGT);
            break;
        case TK_GTE:
            EMIT(RGE);
            break;
        default:
            panic("Unknown real binary operation.");
        }
    }

    MARK(short_circuit);

    JUMP_FIX(short_circuit);
    JUMP_FREE(short_circuit);
}

void eval_unary(ast_unary_t* ast)
{
    eval(ast->expr);
    type_t type = ast->base->type;

    if (is_integer_type(type) || is_bool_type(type))
    {
        switch (ast->op)
        {
        case TK_PLUS:
            break;
        case TK_MINUS:
            EMIT(INEG);
            break;
        case TK_NOT:
            EMIT(INOT);
            break;
        default:
            panic("Unknown unary integer operation.");
        }
    }
    else if (is_real_type(type))
    {
        switch (ast->op)
        {
        case TK_PLUS:
            break;
        case TK_MINUS:
            EMIT(RNEG);
            break;
        default:
            panic("Unknown unary real operation.");
        }
    }
}

void eval_block(ast_block_t* ast)
{
    if (context_is_global(ast->context))
    {
        uint16_t vars = context_allocated(ast->context);
        uint16_t args = 0;
        EMIT(ICONST_0, ICONST_0, PROC, NUM16(args), NUM16((vars - args)));
    }

    for (size_t i = 0; i < vec_size(ast->nodes); i++)
        eval(vec_get(ast->nodes, i));

    // if (context_is_global(ast->context))
        // halt();
}

void eval_single_opcode(ast_single_opcode_t* ast)
{
    EMIT(ast->opcode);
}

void eval_if_cond(ast_if_cond_t* ast)
{
    JUMP_NEW(else_addr);
    JUMP_NEW(exit_addr);

    eval(ast->condition);

    JUMP(JEZ, else_addr);

    eval(ast->if_then);

    JUMP(JMP, exit_addr);

    MARK(else_addr);

    if (ast->if_else != NULL)
        eval(ast->if_else);

    MARK(exit_addr);

    JUMP_FIX(else_addr);
    JUMP_FIX(exit_addr);
    JUMP_FREE(else_addr);
    JUMP_FREE(exit_addr);
}

void eval_assign(ast_assign_t* ast)
{
    eval(ast->expr);
    type_t var_type = ast->symbol->type;
    uint16_t addr = ast->symbol->addr;

    if (ast->index_expr)
    {
        eval(ast->index_expr);
        EMIT(XSTOREI, NUM16(ast->symbol->addr));
    }
    else if (is_array_type(var_type))
    {
        type_t elmnt_type = ast->symbol->extra.array.elmnt_type;
        size_t array_len = ast->symbol->extra.array.len;
        EMIT(ASTORE, NUM16(addr), NUM16(array_len), NUM8(elmnt_type));
    } else {
        EMIT(XSTORE, NUM16(addr));
    }

    if (!ast->new_variable)
    {
        EMIT(ALLC);
    }
}

void eval_variable(ast_variable_t* ast)
{
    type_t var_type = ast->symbol->type;
    uint16_t addr = ast->symbol->addr;

    if (ast->index_expr)
    {
        eval(ast->index_expr);
        EMIT(XLOADI, NUM16(ast->symbol->addr));
    }
    else
    {
        EMIT(XLOAD, NUM16(addr));
    }
}

void eval_func_decl(ast_func_decl_t* ast)
{
    JUMP_NEW(func_end);
    JUMP_NEW(func_beg);
    JUMP(JMP, func_end);
    MARK(func_beg);

    uint16_t vars = context_allocated(ast->body->context);
    uint16_t args = ast->args;
    EMIT(PROC, NUM16(args), NUM16((vars - args)));

    ast->symbol->addr = func_beg->label;

    eval((ast_t*) ast->body);

    EMIT(ICONST_0, RET);
    MARK(func_end);

    JUMP_FIX(func_end);
    JUMP_FIX(func_beg);
    JUMP_FREE(func_end);
    JUMP_FREE(func_beg);
}

void eval_func_call(ast_func_call_t* ast)
{
    for (size_t i = 0; i < vec_size(ast->args); i++)
    {
        eval(vec_get(ast->args, i));
    }
    
    EMIT(CALL, NUM16(ast->symbol->addr));
}

void eval_builtin_call(ast_builtin_call_t* ast)
{
    const builtin_func_t* builtin = builtin_lookup(ast->name);

    if (builtin == NULL)
    {
        panic("Builtin function not found.");
    }

    if (strcmp(builtin->name, "print") == 0)
    {
        for (size_t i = 0; i < vec_size(ast->args); i++)
        {
            ast_t* arg = vec_get(ast->args, i);
            type_t arg_type = arg->base->type;

            eval(arg);
            
            if (is_integer_type(arg_type) || is_bool_type(arg_type))
            {
                EMIT(IPRINT, NUM8(arg_type));
            }
            else if (is_real_type(arg_type))
            {
                EMIT(RPRINT);
            }
            else if (is_str_type(arg_type))
            {
                EMIT(SPRINT);
            }
            else
            {
                panic("Print error. Unknown type.");
            }
        }

        EMIT(ALLC); // TODO: Do we need an code optimization? or less code generation?

        return;
    }

    for (size_t i = 0; i < vec_size(ast->args); i++)
    {
        eval(vec_get(ast->args, i));

        type_t arg_type = ((ast_t*)vec_get(ast->args, i))->base->type;
        
        // Check if the argument type is acceptable
        if (!is_builtin_type_acceptable(arg_type, builtin->acceptable_types))
        {
            panic("Builtin function argument type mismatch.");
        }
    }

    EMIT(builtin->opcode);
}

void eval_func_return(ast_func_return_t* ast)
{
    eval(ast->expr);
    EMIT(RET);
}

void eval_for_loop(ast_for_loop_t* ast)
{
    if (ast->loop == NULL)
        return;

    eval(ast->init);
    MARK(ast->loop->begin);
    eval(ast->condition);
    JUMP(JEZ, ast->loop->end);
    eval(ast->body);
    MARK(ast->loop->post);
    eval(ast->post);
    JUMP(JMP, ast->loop->begin);
    MARK(ast->loop->end);
    
    JUMP_FIX(ast->loop->begin);
    JUMP_FIX(ast->loop->end);
    JUMP_FIX(ast->loop->post);
    JUMP_FREE(ast->loop->begin);
    JUMP_FREE(ast->loop->end);
    JUMP_FREE(ast->loop->post);
}

void eval_break_loop(ast_break_loop_t* ast)
{
    JUMP(JMP, ast->loop->end);
}

void eval_continue_loop(ast_continue_loop_t* ast)
{
    JUMP(JMP, ast->loop->post);
}

void eval_array_scalar(ast_array_scalar_t* ast)
{
    for (size_t i = 0; i < vec_size(ast->elmnts); i++)
    {
        eval(vec_get(ast->elmnts, i));
    }
}

ast_t* ast_new(type_t type, eval_t eval)
{
    ast_t* ast = malloc(sizeof (ast_t));
    ast->base = NULL;
    ast->eval = eval;
    ast->type = type;
    return ast;
}

ast_constant_t* ast_new_constant(type_t type, value_t value)
{
    ast_constant_t* ast_constant = malloc(sizeof (ast_constant_t));
    ast_constant->base = ast_new(type, (eval_t)eval_constant);
    ast_constant->value = value;
    return ast_constant;
}

ast_unary_t* ast_new_unary(type_t type, token_type_t op, ast_t* expr)
{
    ast_unary_t* ast_unary = malloc(sizeof (ast_unary_t));
    ast_unary->base = ast_new(type, (eval_t) eval_unary);
    ast_unary->expr = expr;
    ast_unary->op = op;
    return ast_unary;
}

ast_binary_t* ast_new_binary(type_t type, token_type_t op, ast_t* lhs_expr, ast_t* rhs_expr)
{
    ast_binary_t* ast_binary = malloc(sizeof (ast_binary_t));
    ast_binary->base = ast_new(type, (eval_t) eval_binary);
    ast_binary->lhs_expr = lhs_expr;
    ast_binary->rhs_expr = rhs_expr;
    ast_binary->op = op;
    return ast_binary;
}

ast_block_t* ast_new_block(type_t type, context_t* context)
{
    ast_block_t* ast_block = malloc(sizeof (ast_block_t));
    ast_block->base = ast_new(type, (eval_t) eval_block);
    ast_block->context = context;
    ast_block->nodes = vec_new(0);
    return ast_block;
}

ast_single_opcode_t* ast_new_single_opcode(type_t type, uint8_t opcode)
{
    ast_single_opcode_t* ast_single_opcode = malloc(sizeof (ast_single_opcode_t));
    ast_single_opcode->base = ast_new(type, (eval_t) eval_single_opcode);
    ast_single_opcode->opcode = opcode;
    return ast_single_opcode;
}

ast_if_cond_t* ast_new_if_cond(type_t type, ast_t* condition, ast_t* if_then, ast_t* if_else)
{
    ast_if_cond_t* ast_if_cond = malloc(sizeof (ast_if_cond_t));
    ast_if_cond->base = ast_new(type, (eval_t) eval_if_cond);
    ast_if_cond->condition = condition;
    ast_if_cond->if_then = if_then;
    ast_if_cond->if_else = if_else;
    return ast_if_cond;
}

ast_assign_t* ast_new_assign(type_t type, symbol_t* symbol, ast_t* expr, ast_t* index_expr, bool_t new_variable)
{
    ast_assign_t* ast_assign = malloc(sizeof (ast_assign_t));
    ast_assign->base = ast_new(type, (eval_t) eval_assign);
    ast_assign->symbol = symbol;
    ast_assign->expr = expr;
    ast_assign->index_expr = index_expr;
    ast_assign->new_variable = new_variable;
    return ast_assign;
}

ast_variable_t* ast_new_variable(type_t type, symbol_t* symbol, ast_t* index_expr)
{
    ast_variable_t* ast_variable = malloc(sizeof (ast_variable_t));
    ast_variable->base = ast_new(type, (eval_t) eval_variable);
    ast_variable->symbol = symbol;
    ast_variable->index_expr = index_expr;
    return ast_variable;
}

ast_func_decl_t* ast_new_func_decl(type_t type, symbol_t* symbol, ast_block_t* body, uint16_t args)
{
    ast_func_decl_t* ast_func_decl = malloc(sizeof (ast_func_decl_t));
    ast_func_decl->base = ast_new(type, (eval_t) eval_func_decl);
    ast_func_decl->symbol = symbol;
    ast_func_decl->body = body;
    ast_func_decl->args = args;
    ast_func_decl->ret_type = type;
    return ast_func_decl;
}

ast_func_call_t* ast_new_func_call(type_t type, symbol_t* symbol, vector_t* args)
{
    ast_func_call_t* ast_func_call = malloc(sizeof (ast_func_call_t));
    ast_func_call->base = ast_new(type, (eval_t) eval_func_call);
    ast_func_call->symbol = symbol;
    ast_func_call->args = args;
    return ast_func_call;
}

ast_builtin_call_t* ast_new_builtin_call(type_t type, const char* name, vector_t* args)
{
    ast_builtin_call_t* ast_builtin_call = malloc(sizeof (ast_builtin_call_t));
    ast_builtin_call->base = ast_new(type, (eval_t) eval_builtin_call);
    ast_builtin_call->name = name;
    ast_builtin_call->args = args;
    return ast_builtin_call;
}

ast_func_return_t* ast_new_func_return(type_t type, ast_t* expr)
{
    ast_func_return_t* ast_func_return = malloc(sizeof (ast_func_return_t));
    ast_func_return->base = ast_new(type, (eval_t) eval_func_return);
    ast_func_return->expr = expr;
    return ast_func_return;
}

ast_for_loop_t* ast_new_for_loop(type_t type, ast_t* init, ast_t* condition, ast_t* post, ast_t* body)
{
    ast_for_loop_t* ast_for_loop = malloc(sizeof (ast_for_loop_t));
    ast_for_loop->base = ast_new(type, (eval_t) eval_for_loop);
    ast_for_loop->init = init;
    ast_for_loop->condition = condition;
    ast_for_loop->post = post;
    ast_for_loop->body = body;
    ast_for_loop->loop = body == NULL ? NULL : context_get_loop(((ast_block_t*) body)->context);
    return ast_for_loop;
}

ast_break_loop_t* ast_new_break_loop(type_t type, loop_t* loop)
{
    ast_break_loop_t* ast_break_loop = malloc(sizeof (ast_break_loop_t));
    ast_break_loop->base = ast_new(type, (eval_t) eval_break_loop);
    ast_break_loop->loop = loop;
    return ast_break_loop;
}

ast_continue_loop_t* ast_new_continue_loop(type_t type, loop_t* loop)
{
    ast_continue_loop_t* ast_continue_loop = malloc(sizeof (ast_continue_loop_t));
    ast_continue_loop->base = ast_new(type, (eval_t) eval_continue_loop);
    ast_continue_loop->loop = loop;
    return ast_continue_loop;
}

ast_array_scalar_t* ast_new_array_scalar(type_t type, type_t elmnt_type, vector_t* elmnts)
{
    ast_array_scalar_t* ast_array_scalar = malloc(sizeof (ast_array_scalar_t));
    ast_array_scalar->base = ast_new(type, (eval_t) eval_array_scalar);
    ast_array_scalar->elmnts = elmnts;
    ast_array_scalar->elmnt_type = elmnt_type;
    return ast_array_scalar;
}
