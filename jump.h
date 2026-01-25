#ifndef JUMP_H
#define JUMP_H

#include "types.h"
#include "vector.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MARK(X) jump_label(X)
#define JUMP(OP, X) jump_to(OP, X)
#define JUMP_NEW(X) jump_t* X = jump_new()
#define JUMP_FREE(X) jump_free(X)
#define JUMP_FIX(X) jump_fix(X)

typedef struct
{
    vector_t* jumps;
    uint16_t label;
} jump_t;


jump_t* jump_new();
void jump_free(jump_t* jump);
void jump_to(uint8_t op, jump_t* jump);
void jump_label(jump_t* jump);
void jump_fix(jump_t* jump);

#ifdef __cplusplus
}
#endif

#endif /* JUMP_H */
