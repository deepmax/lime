#include "panic.h"
#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>

void panic(const char* msg)
{
    fprintf(stderr, "%s : %ld %ld", msg, lexer_row() + 1, lexer_col() + 1);
    exit(0);
}

void panic_at(const char* msg, size_t lrow, size_t lcol)
{
    fprintf(stderr, "%s : %ld %ld", msg, lrow, lcol);
    exit(0);
}
