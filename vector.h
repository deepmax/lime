#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    void* data;
} vector_node_t;

typedef struct
{
    vector_node_t* nodes;
    size_t vect_size;
    size_t vect_used;
} vector_t;

vector_t* vec_new(size_t vector_size);
vector_t* vec_clone(vector_t* vector);
void vec_free(vector_t* vector);
void vec_resize(vector_t* vector, size_t vector_size);
void vec_shrink(vector_t* vector);
void vec_set(vector_t* vector, size_t index, void* data);
void* vec_get(vector_t* vector, size_t index);
void* vec_first(vector_t* vector);
void* vec_last(vector_t* vector);
void* vec_append(vector_t* vector, void* data);
size_t vec_size(vector_t* vector);

#ifdef __cplusplus
}
#endif

#endif /* VECTOR_H */
