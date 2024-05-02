#include "./cdilla_lexer.h"

Cdilla_Token_Literal cdilla_token_literals[] = {
    // Symbols
    { .text = SV("("), .kind = CDILLA_TOKEN_OPEN_PAREN },
    { .text = SV(")"), .kind = CDILLA_TOKEN_CLOSE_PAREN },
    { .text = SV("{"), .kind = CDILLA_TOKEN_OPEN_CURLY },
    { .text = SV("}"), .kind = CDILLA_TOKEN_CLOSE_CURLY },
    { .text = SV(";"), .kind = CDILLA_TOKEN_SEMI_COLON },

    // Keywords
    { .text = SV("proc"), .kind = CDILLA_TOKEN_PROC },
    { .text = SV("print"), .kind = CDILLA_TOKEN_PRINT },
};

const char *cdilla_token_kind_cstr_impl(Cdilla_Token_Kind kind, const char *file, int line) {
    switch (kind) {
    case CDILLA_TOKEN_UNKNOWN:         return "<unknown token>";
    case CDILLA_TOKEN_UNCLOSED_STRING: return "<unclosed string>";
    case CDILLA_TOKEN_IDENTIFIER:      return "identifier";
    case CDILLA_TOKEN_INTEGER:         return "literal integer";
    case CDILLA_TOKEN_STRING:          return "literal string";
    case CDILLA_TOKEN_PROC:            return "proc";
    case CDILLA_TOKEN_PRINT:           return "print";
    case CDILLA_TOKEN_OPEN_PAREN:      return "(";
    case CDILLA_TOKEN_CLOSE_PAREN:     return ")";
    case CDILLA_TOKEN_OPEN_CURLY:      return "{";
    case CDILLA_TOKEN_CLOSE_CURLY:     return "}";
    case CDILLA_TOKEN_END:             return "<end of file>";
    case CDILLA_TOKEN_SEMI_COLON:      return ";";
    }
    fprintf(
        stderr, "%s:%d: Error: trying to convert uknown token kind to cstr: %d",
        file, line, kind);
    exit(1);
}

Cdilla_Lexer cdilla_lexer_new(String_View content, const char *source_filepath) {
    Cdilla_Lexer lexer = {0};
    lexer.filepath = source_filepath;
    lexer.content = content;
    lexer.line = 1;
    return lexer;
}

String_View cdilla_lexer_cut_impl(Cdilla_Lexer *lexer, size_t count, const char *file, int line) {
    if (lexer->index + count > lexer->content.count) {
        fprintf(stderr, "%s:%d: Error: trying to cut out of lexer bounds", file, line);
        exit(1);
    }

    String_View head = {
        .data = &lexer->content.data[lexer->index],
        .count = count,
    };

    for (size_t i = 0; i < count; ++i) {
        char ch = lexer->content.data[lexer->index];
        lexer->index += 1;
        if (ch == '\n') {
            lexer->line += 1;
            lexer->bol = lexer->index;
        }
    }
    return head;
}

String_View cdilla_lexer_cut_while(Cdilla_Lexer *lexer, int (*predicate)(int)) {
    size_t count = 0;
    while (lexer->index < lexer->content.count && predicate(lexer->content.data[lexer->index + count])) {
        count += 1;
    }
    return cdilla_lexer_cut(lexer, count);
}

bool cdilla_lexer_starts_with(Cdilla_Lexer *lexer, String_View prefix) {
    if (lexer->index + prefix.count > lexer->content.count) {
        return false;
    }
    return memcmp(&lexer->content.data[lexer->index], prefix.data, prefix.count) == 0;
}

Cdilla_Token cdilla_lexer_next(Cdilla_Lexer *lexer) {
    // TODO: lex comments
    cdilla_lexer_cut_while(lexer, isspace);

    if (lexer->index >= lexer->content.count) {
        return (Cdilla_Token) {
            .kind = CDILLA_TOKEN_END,
            .text = SV("<end>"),
        };
    }

    Cdilla_Loc loc = {
        .filepath = lexer->filepath,
        .row = lexer->line,
        .column = lexer->index - lexer->bol + 1,
    };

    for (size_t i = 0; i < array_len(cdilla_token_literals); ++i) {
        Cdilla_Token_Literal *literal = &cdilla_token_literals[i];
        if (cdilla_lexer_starts_with(lexer, literal->text)) {
            String_View text = cdilla_lexer_cut(lexer, literal->text.count);
            return (Cdilla_Token) {
                .kind = literal->kind,
                .text = text,
                .loc = loc,
            };
        }
    }

    if (isalpha(lexer->content.data[lexer->index])) {
        String_View text = cdilla_lexer_cut_while(lexer, isalnum);
        return (Cdilla_Token) {
            .text = text,
            .kind = CDILLA_TOKEN_IDENTIFIER,
            .loc = loc,
        };
    }

    if (isdigit(lexer->content.data[lexer->index])) {
        String_View text = cdilla_lexer_cut_while(lexer, isdigit);
        return (Cdilla_Token) {
            .text = text,
            .kind = CDILLA_TOKEN_INTEGER,
            .loc = loc,
        };
    }

    if (lexer->content.data[lexer->index] == '"') {
        size_t count = 1;
        Cdilla_Token_Kind kind = CDILLA_TOKEN_UNCLOSED_STRING;
        while (lexer->index + count < lexer->content.count) {
            char ch = lexer->content.data[lexer->index + count];
            if (ch == '\n') {
                break;
            }
            count += 1;
            if (ch == '"') {
                kind = CDILLA_TOKEN_STRING;
                break;
            }
            if (ch == '\\') {
                if (lexer->index + count >= lexer->content.count) {
                    break;
                }
                count += 1;
            }
        }
        String_View text = cdilla_lexer_cut(lexer, count);
        return (Cdilla_Token) { text, kind, loc };
    }

    String_View text = cdilla_lexer_cut(lexer, 1);
    return (Cdilla_Token) {
        .text = text,
        .kind = CDILLA_TOKEN_UNKNOWN,
        .loc = loc,
    };
}
