#include "jump.h"
#include "vector.h"
#include "vm.h"
#include <stdlib.h>

jump_t* jump_new()
{
    jump_t* jump = malloc(sizeof(jump_t));
    jump->jumps = vec_new(0);
    jump->label = 0;
    return jump;
}

void jump_free(jump_t* jump)
{
    vec_free(jump->jumps);
    free(jump);
}

void jump_to(uint8_t op, jump_t* jump)
{
    EMIT(op);
    size_t* addr = malloc(sizeof(size_t));
    *addr = vm_code_addr();
    vec_append(jump->jumps, addr);
    EMIT(NUM16(0));
}

void jump_label(jump_t* jump)
{
    jump->label = vm_code_addr();
}

void jump_fix(jump_t* jump)
{
    for (size_t i = 0; i < vec_size(jump->jumps); i++)
        CODE(*((size_t*)vec_get(jump->jumps, i)), NUM16(jump->label));
}
