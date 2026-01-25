#ifndef BUFFER_H
#define BUFFER_H

#include "types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t* data;
    size_t allc;
    size_t used;
} buffer_t;

void buffer_init(buffer_t* array, size_t allc);
void buffer_free(buffer_t* array);
void buffer_clear(buffer_t* array);
void buffer_shrink(buffer_t* array);
void buffer_set(buffer_t* array, size_t index, uint8_t value);
void buffer_sets(buffer_t* array, size_t index, uint8_t* values, size_t len);
uint8_t buffer_get(buffer_t* array, size_t index);
size_t buffer_size(buffer_t* array);
size_t buffer_add(buffer_t* array, uint8_t value);
size_t buffer_adds(buffer_t* array, uint8_t* values, size_t len);
size_t buffer_alloc(buffer_t* array, uint8_t value, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BUFFER_H */
