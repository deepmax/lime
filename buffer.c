#include "buffer.h"
#include "types.h"
#include <malloc.h>

void buffer_init(buffer_t* array, size_t allc)
{
    if (allc == 0)
        allc = 1;
    array->data = malloc(sizeof (uint8_t) * allc);
    array->allc = allc;
    array->used = 0;
}

void buffer_free(buffer_t* array)
{
    free(array->data);
}

void buffer_set(buffer_t* array, size_t index, uint8_t value)
{
    array->data[index] = value;
}

void buffer_sets(buffer_t* array, size_t index, uint8_t* values, size_t len)
{
    for (size_t i = 0; i < len; i++)
        array->data[i + index] = values[i];
}

uint8_t buffer_get(buffer_t* array, size_t index)
{
    return array->data[index];
}

void buffer_shrink(buffer_t* array)
{
    array->data = realloc(array->data, array->used);
    array->allc = array->used;
}

void buffer_clear(buffer_t* array)
{
    array->data = realloc(array->data, 1);
    array->allc = 1;
    array->used = 0;
}

size_t buffer_size(buffer_t* array)
{
    return array->used;
}

size_t buffer_add(buffer_t* array, uint8_t value)
{
    if (array->used >= array->allc)
    {
        array->allc = min_coverage_size(array->used + 1);
        array->data = realloc(array->data, array->allc * sizeof (uint8_t));
    }
    array->data[array->used++] = value;

    return array->used;
}

size_t buffer_adds(buffer_t* array, uint8_t* values, size_t len)
{
    if (array->used + len >= array->allc)
    {
        array->allc = min_coverage_size(array->used + len);
        array->data = realloc(array->data, array->allc * sizeof (uint8_t));
    }

    for (size_t i = 0; i < len; i++)
        array->data[array->used++] = values[i];

    return array->used;
}

size_t buffer_alloc(buffer_t* array, uint8_t value, size_t len)
{
    if (array->used + len >= array->allc)
    {
        array->allc = min_coverage_size(array->used + len);
        array->data = realloc(array->data, array->allc * sizeof (uint8_t));
    }

    for (size_t i = 0; i < len; i++)
        array->data[array->used++] = value;

    return array->used;
}
