#ifndef CONTEXT_H
#define CONTEXT_H

#include "types.h"
#include "vector.h"
#include "jump.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    const char* id;
    type_t type;
    uint16_t addr;
    union {
        struct {
            type_t ret_type;
            vector_t* param_types;
        } func;
    } extra;
} symbol_t;

typedef struct
{
    jump_t* begin;
    jump_t* end;
    jump_t* post;
} loop_t;

typedef struct context_t
{
    struct context_t* parent;
    block_t block_type;
    vector_t* symbols;
    uint16_t allocated;
    loop_t loop;
} context_t;

context_t* context_new(context_t* parent, block_t block_type);
context_t* context_clone(context_t* context);
void context_free(context_t* context);
symbol_t* context_add(context_t* context, const char* id, type_t type);
symbol_t* context_get(context_t* context, const char* id, bool_t local);
bool_t context_is_global(context_t* context);
size_t context_symbols_count(context_t* context);
uint16_t context_allocated(context_t* context);
void context_add_args_count(context_t* context, uint16_t count);
uint16_t context_alloc_stack_addr(context_t* context);
loop_t* context_get_loop(context_t* context);
context_t* context_get_func(context_t* context);

extern context_t * const global_context;

#ifdef __cplusplus
}
#endif

#endif /* CONTEXT_H */
