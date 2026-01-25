#include "vector.h"
#include <string.h>
#include <stdlib.h>

vector_t* vec_new(size_t vector_size)
{
    vector_t* vector = malloc(sizeof (vector_t));
    vector->vect_size = 0;
    vector->vect_used = 0;
    vector->nodes = NULL;
    vec_resize(vector, vector_size);
    return vector;
}

vector_t* vec_clone(vector_t* vector)
{
    vector_t* cloned = vec_new(vector->vect_used);
    memcpy(cloned->nodes, vector->nodes, vector->vect_used * sizeof(vector_node_t));
    cloned->vect_used = vector->vect_used;
    return cloned;
}

void vec_free(vector_t* vector)
{
    free(vector->nodes);
    free(vector);
}

void vec_resize(vector_t* vector, size_t vector_size)
{
    if (vector->vect_size == vector_size)
        return;
    vector->nodes = realloc(vector->nodes, sizeof(vector_node_t) * vector_size);
    vector->vect_size = vector_size;
    vector->vect_used = vector->vect_used > vector_size ? vector_size : vector->vect_used;
}

void* vec_append(vector_t* vector, void* item)
{
    if (vector->vect_used == vector->vect_size)
        vec_resize(vector, vector->vect_size == 0 ? 1 : vector->vect_size * 2);
    vector->nodes[vector->vect_used].data = item;
    vector->vect_used++;
    return vec_last(vector);
}

void vec_shrink(vector_t* vector)
{
    if (vector->vect_size > vector->vect_used)
        vec_resize(vector, vector->vect_used);
}

void vec_set(vector_t* vector, size_t index, void* item)
{
    vector->nodes[index].data = item;
}

void* vec_get(vector_t* vector, size_t index)
{
    return vector->nodes[index].data;
}

void* vec_first(vector_t* vector)
{
    return vec_get(vector, 0);
}

void* vec_last(vector_t* vector)
{
    return vec_get(vector, vector->vect_used - 1);
}

size_t vec_size(vector_t* vector)
{
    if (vector == NULL)
        return 0;

    return vector->vect_used;
}
