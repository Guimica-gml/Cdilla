#ifndef CDILLA_LEXER_H_
#define CDILLA_LEXER_H_

#include "./utils.h"

#define CDILLA_LOC_FMT "%s:%zu:%zu"
#define CDILLA_LOC_ARG(loc) (loc).filepath, (loc).row, (loc).column

typedef struct {
    const char *filepath;
    String_View content;
    size_t index;
    size_t line;
    size_t bol;
} Cdilla_Lexer;

typedef enum {
    // Errors
    CDILLA_TOKEN_UNKNOWN,
    CDILLA_TOKEN_UNCLOSED_STRING,

    // Special
    CDILLA_TOKEN_IDENTIFIER,
    CDILLA_TOKEN_COMMENT,
    CDILLA_TOKEN_END,

    // Literals
    CDILLA_TOKEN_INTEGER,
    CDILLA_TOKEN_STRING,

    // Keywords
    CDILLA_TOKEN_PROC,
    CDILLA_TOKEN_PRINT,
    CDILLA_TOKEN_LET,

    // Symbols
    CDILLA_TOKEN_OPEN_PAREN,
    CDILLA_TOKEN_CLOSE_PAREN,
    CDILLA_TOKEN_OPEN_CURLY,
    CDILLA_TOKEN_CLOSE_CURLY,
    CDILLA_TOKEN_SEMI_COLON,
    CDILLA_TOKEN_EQUALS,
} Cdilla_Token_Kind;

typedef struct {
    const char *filepath;
    size_t row;
    size_t column;
} Cdilla_Loc;

typedef struct {
    String_View text;
    Cdilla_Token_Kind kind;
    Cdilla_Loc loc;
} Cdilla_Token;

typedef struct {
    String_View text;
    Cdilla_Token_Kind kind;
} Cdilla_Token_Literal;

#define cdilla_lexer_cut_char(lexer) \
    cdilla_lexer_cut_char_loc((lexer), SOURCE_LOC)
#define cdilla_lexer_cut(lexer, count) \
    cdilla_lexer_cut_loc((lexer), (count), SOURCE_LOC)
#define cdilla_token_kind_cstr(kind) \
    cdilla_token_kind_cstr_loc((kind), SOURCE_LOC)

const char *cdilla_token_kind_cstr_loc(Cdilla_Token_Kind kind, Source_Loc loc);

Cdilla_Lexer cdilla_lexer_new(String_View content, const char *source_filepath);
// NOTE(nic): this respects utf8 strings, that's why it returns String_View
String_View cdilla_lexer_cut_char_loc(Cdilla_Lexer *lexer, Source_Loc loc);
String_View cdilla_lexer_cut_loc(Cdilla_Lexer *lexer, size_t count, Source_Loc loc);
String_View cdilla_lexer_cut_while(Cdilla_Lexer *lexer, int (*predicate)(int));
bool cdilla_lexer_starts_with(Cdilla_Lexer *lexer, String_View prefix);
Cdilla_Token cdilla_lexer_next(Cdilla_Lexer *lexer);

#endif // CDILLA_LEXER_H_
