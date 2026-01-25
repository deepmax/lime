#ifndef LEXER_H
#define LEXER_H

#include "types.h"
#include "token.h"

#ifdef __cplusplus
extern "C"
{
#endif

void lexer_load_file(const char* filename);
void lexer_load_stdin();
void lexer_free();
token_t lexer_next();
size_t lexer_row();
size_t lexer_col();

#ifdef __cplusplus
}
#endif

#endif /* LEXER_H */
