#ifndef PARSER_H
#define PARSER_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

void parser_load_file(const char* filename);
void parser_load_stdin();
void parser_free();
void parser_parse();

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
