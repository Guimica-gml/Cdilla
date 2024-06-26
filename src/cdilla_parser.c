#include "./cdilla_parser.h"

// TODO(nic): not stop parsing at first error,
//            keep the errors in a list and parse until the end
// TODO(nic): better error messages

typedef struct {
    char ch;
    char escape_ch;
} Escape_Char_Def;

static Escape_Char_Def escape_chars[] = {
    { .ch = 'n', .escape_ch = '\n' },
    { .ch = 'r', .escape_ch = '\r' },
    { .ch = 't', .escape_ch = '\t' },
    { .ch = '0', .escape_ch = '\0' },
    { .ch = '\\', .escape_ch = '\\' },
    { .ch = '\'', .escape_ch = '\'' },
    { .ch = '\"', .escape_ch = '\"' },
};

Cdilla_Token cdilla_parse_next(Cdilla_Lexer *lexer) {
yet_again:;
    Cdilla_Token token = cdilla_lexer_next(lexer);
    switch (token.kind) {
    case CDILLA_TOKEN_COMMENT: {
        goto yet_again;
    } break;
    case CDILLA_TOKEN_UNKNOWN: {
        fprintf(
            stderr, CDILLA_LOC_FMT": Error: unkown token: "SV_FMT"\n",
            CDILLA_LOC_ARG(token.loc), SV_ARG(token.text));
        exit(1);
    } break;
    case CDILLA_TOKEN_UNCLOSED_STRING: {
        fprintf(
            stderr, CDILLA_LOC_FMT": Error: unclosed string: "SV_FMT"\n",
            CDILLA_LOC_ARG(token.loc), SV_ARG(token.text));
        exit(1);
    } break;
    default: {}
    }
    return token;
}

Cdilla_Token cdilla_parse_expect_impl(Cdilla_Lexer *lexer, Cdilla_Token_Kind kinds[], size_t count) {
    Cdilla_Token token = cdilla_parse_next(lexer);
    for (size_t i = 0; i < count; ++i) {
        if (token.kind == kinds[i]) {
            return token;
        }
    }

    // NOTE(nic): Looks kinda goofy, but does what we need it to do
    fprintf(stderr, CDILLA_LOC_FMT": Error: expected ", CDILLA_LOC_ARG(token.loc));
    for (size_t i = 0; i < count; ++i) {
        fprintf(stderr, "`%s`", cdilla_token_kind_cstr(kinds[i]));
        const char *end = (i == count - 2) ? " or " : ", ";
        fprintf(stderr, "%s", end);
    }
    fprintf(stderr, "but got `%s`\n", cdilla_token_kind_cstr(token.kind));
    exit(1);
}

i64 sv_to_i64(String_View sv) {
    i64 result = 0;
    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        result = result * 10 + (i64) sv.data[i] - '0';
    }
    return result;
}

Cdilla_Expr_Id cdilla_parse_expression(Cdilla_Ast *ast, Cdilla_Lexer *lexer) {
    Cdilla_Expr expr = {0};

    Cdilla_Token token = cdilla_parse_expect(
        lexer,
        CDILLA_TOKEN_INTEGER,
        CDILLA_TOKEN_STRING,
        CDILLA_TOKEN_IDENTIFIER);
    expr.loc = token.loc;

    switch (token.kind) {
    case CDILLA_TOKEN_IDENTIFIER: {
        expr.kind = CDILLA_EXPR_IDENTIFIER;
        expr.as.ident = token.text;
    } break;
    case CDILLA_TOKEN_INTEGER: {
        i64 int64 = sv_to_i64(token.text);
        expr.kind = CDILLA_EXPR_I64;
        expr.as.int64 = int64;
    } break;
    case CDILLA_TOKEN_STRING: {
        size_t begin = da_count(&ast->strings);
        size_t i = 1;

        while (i < token.text.count - 1) {
            char ch = token.text.data[i++];
            if (ch == '\\') {
                char next_ch = token.text.data[i++];
                bool exists = false;
                for (size_t j = 0; j < array_len(escape_chars); ++j) {
                    if (escape_chars[j].ch == next_ch) {
                        da_append(&ast->strings, escape_chars[j].escape_ch);
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    fprintf(
                        stderr,
                        CDILLA_LOC_FMT": Error: escape sequence `\\%c` is not supported\n",
                        CDILLA_LOC_ARG(token.loc), next_ch);
                    exit(1);
                }
            } else {
                da_append(&ast->strings, ch);
            }
        }
        char ch = '\0';
        da_append(&ast->strings, ch);

        expr.kind = CDILLA_EXPR_STRING;
        expr.as.string_index = begin;
    } break;
    default: assert(0 && "unreachable");
    }

    return da_append(&ast->exprs, expr);
}

Cdilla_Code_Block_Id cdilla_parse_code_block(Cdilla_Ast *ast, Cdilla_Lexer *lexer) {
    Cdilla_Code_Block code_block = {0};
    cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_CURLY);

    Cdilla_Token token = cdilla_parse_expect(
        lexer,
        CDILLA_TOKEN_PRINT,
        CDILLA_TOKEN_IDENTIFIER,
        CDILLA_TOKEN_LET,
        CDILLA_TOKEN_CLOSE_CURLY);

    while (token.kind != CDILLA_TOKEN_CLOSE_CURLY) {
        Cdilla_Stmt stmt = {0};
        switch (token.kind) {
        case CDILLA_TOKEN_PRINT: {
            cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_PAREN);
            Cdilla_Expr_Id expr_id = cdilla_parse_expression(ast, lexer);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_CLOSE_PAREN);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_SEMI_COLON);

            stmt.loc = token.loc;
            stmt.kind = CDILLA_STMT_PRINT;
            stmt.as.print = (Cdilla_Stmt_As_Print) {
                expr_id,
            };
        } break;
        case CDILLA_TOKEN_IDENTIFIER: {
            cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_PAREN);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_CLOSE_PAREN);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_SEMI_COLON);

            stmt.loc = token.loc;
            stmt.kind = CDILLA_STMT_PROC_CALL;
            stmt.as.proc_call = (Cdilla_Stmt_As_Proc_Call) {
                token.text,
            };
        } break;
        case CDILLA_TOKEN_LET: {
            Cdilla_Token ident = cdilla_parse_expect(lexer, CDILLA_TOKEN_IDENTIFIER);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_EQUALS);
            Cdilla_Expr_Id expr_id = cdilla_parse_expression(ast, lexer);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_SEMI_COLON);

            stmt.loc = token.loc;
            stmt.kind = CDILLA_STMT_LET;
            stmt.as.let = (Cdilla_Stmt_As_Let) {
                ident.text, expr_id
            };
        } break;
        default: assert(0 && "unreachable");
        }
        da_append(&code_block, stmt);
        token = cdilla_parse_next(lexer);
    }

    return da_append(&ast->code_blocks, code_block);
}

Cdilla_Ast cdilla_parse(Cdilla_Lexer *lexer) {
    Cdilla_Ast ast = {0};
    bool stop = false;
    while (!stop) {
        Cdilla_Token token = cdilla_parse_expect(lexer, CDILLA_TOKEN_PROC, CDILLA_TOKEN_END);
        switch (token.kind) {
        case CDILLA_TOKEN_PROC: {
            Cdilla_Token ident = cdilla_parse_expect(lexer, CDILLA_TOKEN_IDENTIFIER);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_OPEN_PAREN);
            cdilla_parse_expect(lexer, CDILLA_TOKEN_CLOSE_PAREN);

            Cdilla_Code_Block_Id block_id = cdilla_parse_code_block(&ast, lexer);
            Cdilla_Proc proc = { ident.text, block_id };
            da_append(&ast.procs, proc);
        } break;
        case CDILLA_TOKEN_END: {
            stop = true;
        } break;
        default: assert(0 && "unreachable");
        }
    }
    return ast;
}

void cdilla_ast_free(Cdilla_Ast *ast) {
    for (size_t i = 0; i < da_count(&ast->code_blocks); ++i) {
        da_free(&ast->code_blocks.items[i]);
    }
    da_free(&ast->code_blocks);
    da_free(&ast->exprs);
    da_free(&ast->procs);
    da_free(&ast->strings);
}

void cdilla_ast_print(Cdilla_Ast *ast) {
    printf("Procedures:\n");
    for (size_t i = 0; i < da_count(&ast->procs); ++i) {
        Cdilla_Proc *proc = &ast->procs.items[i];
        printf(SV_FMT": code_block_id: %zu\n", SV_ARG(proc->name), proc->code_block_id);
    }
    printf("\n");

    printf("Code_Blocks:\n");
    for (size_t i = 0; i < da_count(&ast->code_blocks); ++i) {
        Cdilla_Code_Block *code_block = &ast->code_blocks.items[i];
        printf("%zu:\n", i);
        if (da_count(code_block) == 0) printf("<empty>\n");
        for (size_t j = 0; j < da_count(code_block); ++j) {
            Cdilla_Stmt *stmt = &code_block->items[j];
            switch (stmt->kind) {
            case CDILLA_STMT_PRINT: {
                printf("print: expression_id: %zu\n", stmt->as.print.expr_id);
            } break;
            case CDILLA_STMT_PROC_CALL: {
                printf("proc_call: name: "SV_FMT"\n", SV_ARG(stmt->as.proc_call.name));
            } break;
            default: assert(0 && "unreachable");
            }
        }
        printf("\n");
    }

    printf("Exprs:\n");
    for (size_t i = 0; i < da_count(&ast->exprs); ++i) {
        printf("%zu: ", i);
        Cdilla_Expr *expr = &ast->exprs.items[i];
        switch (expr->kind) {
        case CDILLA_EXPR_I64: {
            printf("Integer: %ld", expr->as.int64);
        } break;
        case CDILLA_EXPR_STRING: {
            printf("String Index: %zu", expr->as.string_index);
        } break;
        default: assert(0 && "unreachable");
        }
        printf("\n");
    }
    printf("\n");

    printf("Strings:\n");
    size_t column_size = 8;
    for (size_t i = 0; i < da_count(&ast->strings); ++i) {
        printf("0x%02X", (u8) ast->strings.items[i]);
        printf("%s", ((i + 1) % column_size == 0) ? "\n" : " ");
    }
    printf("\n");

}
