#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "./utils.h"
#include "./cdilla_lexer.h"

typedef size_t Cdilla_Expression_Id;
typedef size_t Cdilla_Code_Block_Id;

typedef enum {
    CDILLA_EXPRESSION_I32,
    CDILLA_EXPRESSION_STRING,
} Cdilla_Expression_Kind;

typedef union {
    i32 int_32;
    size_t string_index;
} Cdilla_Expression_As;

typedef struct {
    Cdilla_Expression_Kind kind;
    Cdilla_Expression_As as;
} Cdilla_Expression;

typedef enum {
    CDILLA_STATEMENT_PRINT,
} Cdilla_Statement_Kind;

typedef struct {
    Cdilla_Expression_Id expr_id;
} Cdilla_Statement_As_Print;

typedef union {
    Cdilla_Statement_As_Print print;
} Cdilla_Statement_As;

typedef struct {
    Cdilla_Loc loc;
    Cdilla_Statement_Kind kind;
    Cdilla_Statement_As as;
} Cdilla_Statement;

typedef struct {
    Cdilla_Statement *items;
    size_t count;
    size_t capacity;
} Cdilla_Code_Block;

typedef struct {
    String_View name;
    Cdilla_Code_Block_Id code_block_id;
} Cdilla_Procedure;

typedef struct {
    Cdilla_Expression *items;
    size_t count;
    size_t capacity;
} Cdilla_Expressions;

typedef struct {
    Cdilla_Code_Block *items;
    size_t count;
    size_t capacity;
} Cdilla_Code_Blocks;

typedef struct {
    Cdilla_Procedure *items;
    size_t count;
    size_t capacity;
} Cdilla_Procedures;

typedef struct {
    String_Builder strings;
    Cdilla_Expressions expressions;
    Cdilla_Code_Blocks code_blocks;
    Cdilla_Procedures procedures;
} Cdilla_Ast;

Cdilla_Token cdilla_parse_next(Cdilla_Lexer *lexer) {
    Cdilla_Token token = cdilla_lexer_next(lexer);
    switch (token.kind) {
    case CDILLA_TOKEN_UNKNOWN: {
        fprintf(
            stderr, LOC_FMT": Error: unkown token: "SV_FMT"\n",
            LOC_ARG(token.loc), SV_ARG(token.text));
        exit(1);
    } break;
    case CDILLA_TOKEN_UNCLOSED_STRING: {
        fprintf(
            stderr, LOC_FMT": Error: unclosed string: "SV_FMT"\n",
            LOC_ARG(token.loc), SV_ARG(token.text));
        exit(1);
    } break;
    default: {}
    }
    return token;
}

Cdilla_Token cdilla_parse_expect(Cdilla_Lexer *lexer, Cdilla_Token_Kind kind) {
    Cdilla_Token token = cdilla_parse_next(lexer);
    if (token.kind != kind) {
        fprintf(
            stderr,
            LOC_FMT": Error: expected `%s`, but got `%s`\n",
            LOC_ARG(token.loc),
            cdilla_token_kind_cstr(kind),
            cdilla_token_kind_cstr(token.kind));
        exit(1);
    }
    return token;
}

i32 sv_to_i32(String_View sv) {
    i32 result = 0;
    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        result = result * 10 + (i32) sv.data[i] - '0';
    }
    return result;
}

typedef struct {
    char ch;
    char escape_ch;
} Escape_Char_Def;

Escape_Char_Def escape_chars[] = {
    { .ch = 'n', .escape_ch = '\n' },
    { .ch = 'r', .escape_ch = '\r' },
    { .ch = 't', .escape_ch = '\t' },
    { .ch = '0', .escape_ch = '\0' },
    { .ch = '\\', .escape_ch = '\\' },
    { .ch = '\'', .escape_ch = '\'' },
    { .ch = '\"', .escape_ch = '\"' },
};

Cdilla_Expression_Id cdilla_parse_expression(Cdilla_Ast *ast, Cdilla_Lexer *lexer) {
    Cdilla_Expression expression = {0};

    Cdilla_Token token = cdilla_parse_next(lexer);
    switch (token.kind) {
    case CDILLA_TOKEN_INTEGER: {
        i32 int_32 = sv_to_i32(token.text);
        expression.kind = CDILLA_EXPRESSION_I32;
        expression.as.int_32 = int_32;
    } break;
    case CDILLA_TOKEN_STRING: {
        char string[token.text.count + 1];
        size_t len = 0;

        size_t i = 1;
        while (i < token.text.count - 1) {
            char ch = token.text.data[i++];
            if (ch == '\\') {
                char next_ch = token.text.data[i++];
                bool exists = false;
                for (size_t j = 0; j < array_len(escape_chars); ++j) {
                    if (escape_chars[j].ch == next_ch) {
                        string[len++] = escape_chars[j].escape_ch;
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    fprintf(
                        stderr,
                        LOC_FMT": Error: escape sequence `\\%c` is not supported\n",
                        LOC_ARG(token.loc), next_ch);
                    exit(1);
                }
            } else {
                string[len++] = ch;
            }
        }
        string[len++] = '\0';

        size_t begin = ast->strings.count;
        sb_add_sized_str(&ast->strings, string, len);

        expression.kind = CDILLA_EXPRESSION_STRING;
        expression.as.string_index = begin;
    } break;
    default: {
        fprintf(
            stderr,
            LOC_FMT": Error: expected `%s` or `%s`, but got `%s`\n",
            LOC_ARG(token.loc),
            cdilla_token_kind_cstr(CDILLA_TOKEN_INTEGER),
            cdilla_token_kind_cstr(CDILLA_TOKEN_STRING),
            cdilla_token_kind_cstr(token.kind));
        exit(1);
    } break;
    }

    da_append(&ast->expressions, expression);
    return ast->expressions.count - 1;
}

Cdilla_Code_Block_Id cdilla_parse_code_block(Cdilla_Ast *ast, Cdilla_Lexer *lexer) {
    Cdilla_Code_Block code_block = {0};
    cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_CURLY);

    Cdilla_Token token = cdilla_parse_next(lexer);
    while (token.kind != CDILLA_TOKEN_CLOSE_CURLY) {
        Cdilla_Statement statement = {0};
        switch (token.kind) {
        case CDILLA_TOKEN_PRINT: {
            cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_PAREN);
            Cdilla_Expression_Id expression_id = cdilla_parse_expression(ast, lexer);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_CLOSE_PAREN);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_SEMI_COLON);

            statement.loc = token.loc;
            statement.kind = CDILLA_STATEMENT_PRINT;
            statement.as.print = (Cdilla_Statement_As_Print) {
                expression_id,
            };
        } break;
        default: {
            fprintf(
                stderr,
                LOC_FMT": Error: expected `%s`, but got `%s`\n",
                LOC_ARG(token.loc),
                cdilla_token_kind_cstr(CDILLA_TOKEN_PRINT),
                cdilla_token_kind_cstr(token.kind));
            exit(1);
        } break;
        }
        da_append(&code_block, statement);
        token = cdilla_parse_next(lexer);
    }

    da_append(&ast->code_blocks, code_block);
    return ast->code_blocks.count - 1;
}

Cdilla_Ast cdilla_parse(Cdilla_Lexer *lexer) {
    Cdilla_Ast ast = {0};
    while (true) {
        Cdilla_Token token = cdilla_parse_next(lexer);
        switch (token.kind) {
        case CDILLA_TOKEN_PROC: {
            Cdilla_Token ident = cdilla_parse_expect(lexer, CDILLA_TOKEN_IDENTIFIER);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_PAREN);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_CLOSE_PAREN);

            Cdilla_Code_Block_Id block_id = cdilla_parse_code_block(&ast, lexer);
            Cdilla_Procedure proc = { ident.text, block_id };
            da_append(&ast.procedures, proc);
        } break;
        case CDILLA_TOKEN_END: {
            goto end;
        } break;
        default: {
            fprintf(
                stderr,
                LOC_FMT": Error: expected `%s` or `%s`, but got `%s`\n",
                LOC_ARG(token.loc),
                cdilla_token_kind_cstr(CDILLA_TOKEN_PROC),
                cdilla_token_kind_cstr(CDILLA_TOKEN_END),
                cdilla_token_kind_cstr(token.kind));
            exit(1);
        } break;
        }
    }
end:
    return ast;
}

void cdilla_ast_free(Cdilla_Ast *ast) {
    for (size_t i = 0; i < ast->code_blocks.count; ++i) {
        da_free(&ast->code_blocks.items[i]);
    }
    da_free(&ast->code_blocks);
    da_free(&ast->expressions);
    da_free(&ast->procedures);
    sb_free(&ast->strings);
}

void print_usage(FILE *stream, const char *program) {
    fprintf(stream, "Usage: %s <filepath>", program);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Error: expected source code filepath\n");
        print_usage(stderr, argv[0]);
        exit(1);
    }

    const char *source_filepath = argv[1];
    String_Builder content = {0};

    Errno err = read_file(source_filepath, &content);
    if (err) {
        fprintf(
            stderr, "Error: couldn't read file %s: %s\n",
            source_filepath, strerror(err));
        exit(1);
    }

    String_View code = sv_from_sb(&content);
    Cdilla_Lexer lexer = cdilla_lexer_new(code, source_filepath);
    Cdilla_Ast ast = cdilla_parse(&lexer);

    cdilla_ast_free(&ast);
    sb_free(&content);
    return 0;
}