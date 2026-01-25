#include "lexer.h"
#include "token.h"
#include "buffer.h"
#include "types.h"
#include "utf8.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static FILE* file;
static bool_t is_stdin;
static char look;
static size_t row;
static size_t col;

typedef struct
{
    char* name;
    token_type_t token_type;
} keyword_t;

static const keyword_t KEYWORDS[] = {
    {"var", TK_VAR},
    {"let", TK_LET},
    {"func", TK_FUNC},
    {"true", TK_TRUE},
    {"false", TK_FALSE},
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"loop", TK_LOOP},
    {"for", TK_FOR},
    {"in", TK_IN},
    {"break", TK_BREAK},
    {"continue", TK_CONTINUE},
    {"return", TK_RETURN},
    {"read", TK_READ},
    {"exit", TK_EXIT},
    {"and", TK_AND},
    {"or", TK_OR},
    {"not", TK_NOT},
    {"of", TK_OF},
    {"extern", TK_EXTERN},
    {"struct", TK_STRUCT},
};

void lexer_load_file(const char* filename)
{
    row = 0;
    col = 0;
    look = 0;
    is_stdin = false;
    file = fopen(filename, "r");
}

void lexer_load_stdin()
{
    row = 0;
    col = 0;
    look = 0;
    is_stdin = true;
    file = stdin;
}

void lexer_free()
{
    if (!is_stdin)
    {
        fclose(file);
    }
}

size_t lexer_row()
{
    return row;
}

size_t lexer_col()
{
    return col;
}

void lexer_skip_white()
{
    while ((look = fgetc(file)) != EOF && isspace(look))
    {
        col++;

        if (look == '\n')
        {
            row++;
            col = 0;
        }
    }
}

void lexer_skip_line_comment()
{
    while (look == '#')
    {
        while ((look = fgetc(file)) != EOF && look != '\n')
        {
        }

        row++;
        col = 0;

        lexer_skip_white();
    }
}

int is_ident_char(int c)
{
    return isalnum(c) || c == '_';
}

int fpeek(FILE *stream)
{
    int c = fgetc(stream);
    ungetc(c, stream);
    return c;
}

token_type_t find_keyword(const char* name)
{
    for (int i = 0; i < sizeof (KEYWORDS) / sizeof (KEYWORDS[0]); i++)
    {
        if (strcmp(KEYWORDS[i].name, name) == 0)
            return KEYWORDS[i].token_type;
    }
    return TK_BAD;
}

token_t lexer_next()
{
    lexer_skip_white();
    lexer_skip_line_comment();

    token_t token;
    token.type = TK_BAD;
    token.row = row;
    token.col = col;
    token.value.as_int64 = 0;

    col++;

    if (feof(file))
    {
        token.type = TK_FIN;
    }
    else if (look == '{')
    {
        token.type = TK_L_BRACE;
    }
    else if (look == '}')
    {
        token.type = TK_R_BRACE;
    }
    else if (look == '(')
    {
        token.type = TK_L_PAREN;
    }
    else if (look == ')')
    {
        token.type = TK_R_PAREN;
    }
    else if (look == '[')
    {
        token.type = TK_L_BRACKET;
    }
    else if (look == ']')
    {
        token.type = TK_R_BRACKET;
    }
    else if (look == ',')
    {
        token.type = TK_COMMA;
    }
    else if (look == ':')
    {
        token.type = TK_COLON;
    }
    else if (look == '.' && fpeek(file) == '.')
    {
        token.type = TK_DOTDOT;
        look = fgetc(file);
    }
    else if (look == '.')
    {
        token.type = TK_PERIOD;
    }
    else if (look == ';')
    {
        token.type = TK_SEMICOLON;
    }
    else if (look == '+')
    {
        token.type = TK_PLUS;
    }
    else if (look == '-')
    {
        token.type = TK_MINUS;
    }
    else if (look == '*')
    {
        token.type = TK_MUL;
    }
    else if (look == '/')
    {
        token.type = TK_DIV;
    }
    else if (look == '%')
    {
        token.type = TK_MOD;
    }
    else if (look == '|')
    {
        token.type = TK_OR_BIT;
    }
    else if (look == '&')
    {
        token.type = TK_AND_BIT;
    }
    else if (look == '^')
    {
        token.type = TK_XOR_BIT;
    }
    else if (look == '\\')
    {
        token.type = TK_BACKSLASH;
    }
    else if (look == '?')
    {
        token.type = TK_QUESTION;
    }
    else if (look == '=' && fpeek(file) == '=')
    {
        token.type = TK_EQ;
        look = fgetc(file);
    }
    else if (look == '=')
    {
        token.type = TK_ASSIGN;
    }
    else if (look == '>' && fpeek(file) == '=')
    {
        token.type = TK_GTE;
        look = fgetc(file);
    }
    else if (look == '>')
    {
        token.type = TK_GT;
    }
    else if (look == '<' && fpeek(file) == '=')
    {
        token.type = TK_LTE;
        look = fgetc(file);
    }
    else if (look == '<')
    {
        token.type = TK_LT;
    }
    else if (look == '!' && fpeek(file) == '=')
    {
        token.type = TK_NE;
        look = fgetc(file);
    }
    else if (isalpha(look) || look == '_')
    {
        buffer_t ident;
        buffer_init(&ident, 32);
        buffer_add(&ident, look);

        while (is_ident_char(fpeek(file)))
        {
            look = fgetc(file);
            col++;
            buffer_add(&ident, look);
        }

        buffer_add(&ident, '\0');
        buffer_shrink(&ident);

        token_type_t t = find_keyword((char*)ident.data);
        token.type = t == TK_BAD ? TK_IDENT : t;
        token.value.as_str = (char*) ident.data;
    }
    else if (isdigit(look))
    {
        bool_t is_real = false;
        bool_t is_unsigned = false;

        buffer_t number;
        buffer_init(&number, 32);
        buffer_add(&number, look);

        while (true)
        {
            char peek = fpeek(file);

            if (isdigit(peek) || (peek == '.' && !is_real))
            {
                if (peek == '.')
                    is_real = true;

                if ((look = fgetc(file)) != EOF)
                {
                    col++;
                    buffer_add(&number, look);
                    continue;
                }
            }
            break;
        }

        {
            char peek = fpeek(file);
            if (peek == 'u' || peek == 'U')
            {
                is_unsigned = true;
                look = fgetc(file);
            }
        }

        buffer_add(&number, '\0');
        buffer_shrink(&number);

        if (is_real)
        {
            uint8_t* end = number.data + number.used - 1;
            token.type = TK_REAL;
            token.value.as_real = strtod((char*) number.data, (char**) &end);
        }
        else
        {
            uint8_t* end = number.data + number.used - 1;

            if (is_unsigned)
            {
                uint64_t val = strtoull((char*) number.data, (char**) &end, 10);

                if (val <= UINT8_MAX) {
                    token.type = TK_UINT8;
                    token.value.as_uint64 = val;
                } else if (val <= UINT16_MAX) {
                    token.type = TK_UINT16;
                    token.value.as_uint64 = val;
                } else if (val <= UINT32_MAX) {
                    token.type = TK_UINT32;
                    token.value.as_uint64 = val;
                } else {
                    token.type = TK_UINT64;
                    token.value.as_uint64 = val;
                }
            }
            else
            {
                int64_t val = strtoll((char*) number.data, (char**) &end, 10);

                if (val >= INT8_MIN && val <= INT8_MAX) {
                    token.type = TK_INT8;
                    token.value.as_int64 = val;
                } else if (val >= INT16_MIN && val <= INT16_MAX) {
                    token.type = TK_INT16;
                    token.value.as_int64 = val;
                } else if (val >= INT32_MIN && val <= INT32_MAX) {
                    token.type = TK_INT32;
                    token.value.as_int64 = val;
                } else {
                    token.type = TK_INT64;
                    token.value.as_int64 = val;
                }
            }
        }
    }
    else if (look == '"')
    {
        buffer_t str;
        buffer_init(&str, 64);

        while (fpeek(file) != '"' && (look = fgetc(file)) != EOF)
        {
            if (look == '\\')
            {
                char escaped = 0;
                switch (fpeek(file))
                {
                case 'n':
                    escaped = '\n';
                    break;
                case 't':
                    escaped = '\t';
                    break;
                case 'a':
                    escaped = '\a';
                    break;
                case 'f':
                    escaped = '\f';
                    break;
                case 'r':
                    escaped = '\r';
                    break;
                case 'v':
                    escaped = '\v';
                    break;
                case '\\':
                    escaped = '\\';
                    break;
                case '"':
                    escaped = '"';
                    break;
                }

                if (escaped != 0)
                {
                    look = escaped;
                    fgetc(file);
                    col++;
                }
            }

            col++;
            buffer_add(&str, look);
        }

        look = fgetc(file);

        if (look != '"')
        {
            token.type = TK_BAD;
        }
        else
        {
            buffer_add(&str, 0);
            buffer_shrink(&str);

            if (utf8valid((utf8_int8_t*)str.data) != NULL)
            {
                token.type = TK_BAD;
            }
            else
            {
                token.type = TK_STR;
                token.value.as_str = (char*) str.data;
            }
        }
    }

    return token;
}
