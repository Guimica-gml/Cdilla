#include "./cdilla_lexer.h"

Cdilla_Token_Literal cdilla_symbol_literals[] = {
    { .text = SV("("), .kind = CDILLA_TOKEN_OPEN_PAREN },
    { .text = SV(")"), .kind = CDILLA_TOKEN_CLOSE_PAREN },
    { .text = SV("{"), .kind = CDILLA_TOKEN_OPEN_CURLY },
    { .text = SV("}"), .kind = CDILLA_TOKEN_CLOSE_CURLY },
    { .text = SV(";"), .kind = CDILLA_TOKEN_SEMI_COLON },
};

Cdilla_Token_Literal cdilla_keyword_literals[] = {
    { .text = SV("proc"), .kind = CDILLA_TOKEN_PROC },
    { .text = SV("print"), .kind = CDILLA_TOKEN_PRINT },
};

int cdilla_is_ident_body(int ch) {
    return isalpha(ch);
}

int cdilla_is_ident_head(int ch) {
    return isalnum(ch);
}

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

size_t utf8_char_size(char ch) {
    if ((ch & (1 << 7)) == 0) return 1;
    if ((ch & (1 << 5)) == 0) return 2;
    if ((ch & (1 << 4)) == 0) return 3;
    if ((ch & (1 << 3)) == 0) return 4;
    assert(0 && "unreachable");
}

size_t utf8_sv_char_count(String_View sv) {
    size_t actual_count = 0;

    size_t i = 0;
    while (i < sv.count) {
        size_t char_size = utf8_char_size(sv.data[i]);
        i += char_size;
        actual_count += 1;
    }

    return actual_count;
}

Cdilla_Lexer cdilla_lexer_new(String_View content, const char *source_filepath) {
    String_View test = SV("ç");
    printf("String view size: %zu\n", test.count);
    printf("Actual size: %zu\n", utf8_sv_char_count(test));

    Cdilla_Lexer lexer = {0};
    lexer.filepath = source_filepath;
    lexer.content = content;
    lexer.line = 1;
    return lexer;
}

String_View cdilla_lexer_cut_char_impl(Cdilla_Lexer *lexer, const char *file, int line) {
    if (lexer->index >= lexer->content.count) {
        fprintf(stderr, "%s:%d: Error: trying to cut out of lexer bounds", file, line);
        exit(1);
    }

    const char *ch = &lexer->content.data[lexer->index];
    size_t char_size = utf8_char_size(*ch);
    lexer->index += char_size;

    if (*ch == '\n') {
        lexer->line += 1;
        lexer->bol = lexer->index;
    }

    return (String_View) { ch, char_size };
}

String_View cdilla_lexer_cut_impl(Cdilla_Lexer *lexer, size_t count, const char *file, int line) {
    const char *begin = &lexer->content.data[lexer->index];
    size_t actual_count = 0;

    for (size_t i = 0; i < count; ++i) {
        String_View ch = cdilla_lexer_cut_char_impl(lexer, file, line);
        actual_count += ch.count;
    }

    return (String_View) { begin, actual_count };
}

String_View cdilla_lexer_cut_while(Cdilla_Lexer *lexer, int (*predicate)(int)) {
    const char *begin = &lexer->content.data[lexer->index];
    size_t actual_count = 0;

    while (lexer->index < lexer->content.count && predicate(lexer->content.data[lexer->index])) {
        String_View ch = cdilla_lexer_cut_char(lexer);
        actual_count += ch.count;
    }

    return (String_View) { begin, actual_count };
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

    Cdilla_Loc loc = {
        .filepath = lexer->filepath,
        .row = lexer->line,
        .column = lexer->index - lexer->bol + 1,
    };

    if (lexer->index >= lexer->content.count) {
        return (Cdilla_Token) {
            .text = SV("<end>"),
            .kind = CDILLA_TOKEN_END,
            .loc = loc,
        };
    }

    for (size_t i = 0; i < array_len(cdilla_symbol_literals); ++i) {
        Cdilla_Token_Literal *literal = &cdilla_symbol_literals[i];
        if (cdilla_lexer_starts_with(lexer, literal->text)) {
            String_View text = cdilla_lexer_cut(lexer, literal->text.count);
            return (Cdilla_Token) { text, literal->kind, loc };
        }
    }

    if (cdilla_is_ident_head(lexer->content.data[lexer->index])) {
        String_View text = cdilla_lexer_cut_while(lexer, cdilla_is_ident_body);
        Cdilla_Token_Kind kind = CDILLA_TOKEN_IDENTIFIER;
        for (size_t i = 0; i < array_len(cdilla_keyword_literals); ++i) {
            Cdilla_Token_Literal *literal = &cdilla_keyword_literals[i];
            if (sv_eq(text, literal->text)) {
                kind = literal->kind;
            }
        }
        return (Cdilla_Token) { text, kind, loc };
    }

    if (isdigit(lexer->content.data[lexer->index])) {
        String_View text = cdilla_lexer_cut_while(lexer, isdigit);
        return (Cdilla_Token) { text, CDILLA_TOKEN_INTEGER, loc };
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

    String_View text = cdilla_lexer_cut_char(lexer);
    return (Cdilla_Token) { text, CDILLA_TOKEN_UNKNOWN, loc };
}
