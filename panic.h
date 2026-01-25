#ifndef PANIC_H
#define PANIC_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

void panic(const char* msg);
void panic_at(const char* msg, size_t lrow, size_t lcol);

#ifdef __cplusplus
}
#endif

#endif /* PANIC_H */
