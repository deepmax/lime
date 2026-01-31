#include "context.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

context_t * const global_context = &(context_t){NULL, MB_GLOBAL, NULL};

context_t* context_new(context_t* parent, block_t block_type)
{
    context_t* context = malloc(sizeof (context_t));
    context->symbols = NULL;
    context->parent = parent;
    context->allocated = 0;
    context->block_type = block_type;
    if (block_type == MB_LOOP)
    {
        context->loop.begin = jump_new();
        context->loop.end = jump_new();
        context->loop.post = jump_new();
    }
    return context;
}

context_t* context_clone(context_t* context)
{
    context_t* cloned = malloc(sizeof (context_t));
    cloned->parent = context->parent;
    cloned->block_type = context->block_type;
    cloned->symbols = vec_clone(context->symbols);
    cloned->allocated = context->allocated;
    return cloned;
}

void context_free(context_t* context)
{
    vec_free(context->symbols);
    free(context);
}

bool_t context_is_global(context_t* context)
{
    return context == global_context;
}

symbol_t* context_add(context_t* context, const char* id, type_t type)
{
    symbol_t* symbol = context_get(context, id, true);
    if (symbol != NULL)
    {
        symbol->type = type;
        // symbol->addr = vec_size(context->symbols);
        return symbol;
    }

    symbol_t* new_symbol = malloc(sizeof (symbol_t));
    new_symbol->id = id;
    new_symbol->type = type;
    new_symbol->addr_on_stack = context_alloc_stack_addr(context, 1);
    new_symbol->extra.func.ret_type = MT_UNKNOWN;
    new_symbol->extra.func.param_types = NULL;

    if (context->symbols == NULL)
        context->symbols = vec_new(0);

    return (symbol_t*) vec_append(context->symbols, new_symbol);
}

symbol_t* context_get(context_t* context, const char* id, bool_t local)
{
    for (context_t* c = context; c != NULL; c = c->parent)
    {
        for (size_t i = 0; i < vec_size(c->symbols); i++)
        {
            symbol_t* s = (symbol_t*) vec_get(c->symbols, i);
            if (strcmp(id, s->id) == 0)
                return s;
        }

        if (local)
            break;

        if (c->block_type == MB_FUNC || c->block_type == MB_CLASS)
            break;
    }

    if (!context_is_global(context) && !local)
    {
        return context_get(global_context, id, false);
    }

    return NULL;
}

size_t context_symbols_count(context_t* context)
{
    return vec_size(context->symbols);
}

uint16_t context_allocated(context_t* context)
{
    return context->allocated;
}

loop_t* context_get_loop(context_t* context)
{
    for (context_t* c = context; c != NULL; c = c->parent)
    {
        if (c->block_type == MB_LOOP)
            return &c->loop;

        if (c->block_type != MB_NORMAL)
            return NULL;
    }
    return NULL;
}

context_t* context_get_func(context_t* context)
{
    for (context_t* c = context; c != NULL; c = c->parent)
    {
        if (c->block_type == MB_FUNC)
            return c;
    }
    return NULL;
}

uint16_t context_alloc_stack_addr(context_t* context, uint16_t size)
{
    context_t* func_context = context_get_func(context);
    if (func_context != NULL)
    {
        func_context->allocated += size;
        return func_context->allocated;
    }
    global_context->allocated += size;
    return global_context->allocated;
}
