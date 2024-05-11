#include "./cdilla_lexer.h"

static String_View cdilla_comment_begin = SV("//");

static Cdilla_Token_Literal cdilla_symbols[] = {
    { .text = SV("("), .kind = CDILLA_TOKEN_OPEN_PAREN },
    { .text = SV(")"), .kind = CDILLA_TOKEN_CLOSE_PAREN },
    { .text = SV("{"), .kind = CDILLA_TOKEN_OPEN_CURLY },
    { .text = SV("}"), .kind = CDILLA_TOKEN_CLOSE_CURLY },
    { .text = SV(";"), .kind = CDILLA_TOKEN_SEMI_COLON },
};

static Cdilla_Token_Literal cdilla_keywords[] = {
    { .text = SV("proc"), .kind = CDILLA_TOKEN_PROC },
    { .text = SV("print"), .kind = CDILLA_TOKEN_PRINT },
};

const char *cdilla_token_kind_cstr_loc(Cdilla_Token_Kind kind, Source_Loc loc) {
    switch (kind) {
    case CDILLA_TOKEN_UNKNOWN:         return "<unknown token>";
    case CDILLA_TOKEN_UNCLOSED_STRING: return "<unclosed string>";
    case CDILLA_TOKEN_IDENTIFIER:      return "identifier";
    case CDILLA_TOKEN_COMMENT:         return "comment";
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
    PANIC(loc, "trying to convert uknown token kind to cstr: %d", kind);
}

#define utf8_char_size(ch) \
    utf8_char_size_loc(ch, SOURCE_LOC)

static size_t utf8_char_size_loc(char ch, Source_Loc loc) {
    if ((ch & (1 << 7)) == 0) return 1;
    if ((ch & (1 << 5)) == 0) return 2;
    if ((ch & (1 << 4)) == 0) return 3;
    if ((ch & (1 << 3)) == 0) return 4;
    PANIC(loc, "either not valid utf8 char or a continuation to utf8 char");
}

Cdilla_Lexer cdilla_lexer_new(String_View content, const char *source_filepath) {
    Cdilla_Lexer lexer = {0};
    lexer.filepath = source_filepath;
    lexer.content = content;
    lexer.line = 1;
    return lexer;
}

String_View cdilla_lexer_cut_char_loc(Cdilla_Lexer *lexer, Source_Loc loc) {
    if (lexer->index >= lexer->content.count) {
        PANIC(loc, "trying to cut out of lexer bounds");
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

String_View cdilla_lexer_cut_loc(Cdilla_Lexer *lexer, size_t count, Source_Loc loc) {
    const char *head = &lexer->content.data[lexer->index];
    size_t byte_count = 0;

    for (size_t i = 0; i < count; ++i) {
        String_View ch = cdilla_lexer_cut_char_loc(lexer, loc);
        byte_count += ch.count;
    }

    return (String_View) { head, byte_count };
}

String_View cdilla_lexer_cut_while(Cdilla_Lexer *lexer, int (*predicate)(int)) {
    const char *head = &lexer->content.data[lexer->index];
    size_t byte_count = 0;

    while (lexer->index < lexer->content.count && predicate(lexer->content.data[lexer->index])) {
        String_View ch = cdilla_lexer_cut_char(lexer);
        byte_count += ch.count;
    }

    return (String_View) { head, byte_count };
}

bool cdilla_lexer_starts_with(Cdilla_Lexer *lexer, String_View prefix) {
    if (lexer->index + prefix.count > lexer->content.count) {
        return false;
    }
    return memcmp(&lexer->content.data[lexer->index], prefix.data, prefix.count) == 0;
}

int cdilla_not_linebreak(int ch) {
    return ch != '\n';
}

Cdilla_Token cdilla_lexer_next(Cdilla_Lexer *lexer) {
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

    if (cdilla_lexer_starts_with(lexer, cdilla_comment_begin)) {
        String_View text = cdilla_lexer_cut_while(lexer, cdilla_not_linebreak);
        return (Cdilla_Token) { text, CDILLA_TOKEN_COMMENT, loc };
    }

    for (size_t i = 0; i < array_len(cdilla_symbols); ++i) {
        Cdilla_Token_Literal *literal = &cdilla_symbols[i];
        if (cdilla_lexer_starts_with(lexer, literal->text)) {
            String_View text = cdilla_lexer_cut(lexer, literal->text.count);
            return (Cdilla_Token) { text, literal->kind, loc };
        }
    }

    if (isalpha(lexer->content.data[lexer->index])) {
        String_View text = cdilla_lexer_cut_while(lexer, isalnum);
        Cdilla_Token_Kind kind = CDILLA_TOKEN_IDENTIFIER;
        for (size_t i = 0; i < array_len(cdilla_keywords); ++i) {
            Cdilla_Token_Literal *literal = &cdilla_keywords[i];
            if (sv_equals(text, literal->text)) {
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
        const char *head = &lexer->content.data[lexer->index];
        cdilla_lexer_cut_char(lexer);

        size_t head_index = lexer->index;
        Cdilla_Token_Kind kind = CDILLA_TOKEN_UNCLOSED_STRING;

        while (lexer->index < lexer->content.count) {
            char ch = lexer->content.data[lexer->index];
            if (ch == '\n') {
                break;
            }
            cdilla_lexer_cut_char(lexer);
            if (ch == '"') {
                kind = CDILLA_TOKEN_STRING;
                break;
            }
            if (ch == '\\') {
                if (lexer->index >= lexer->content.count) {
                    break;
                }
                cdilla_lexer_cut_char(lexer);
            }
        }

        String_View text = { head, lexer->index - head_index };
        return (Cdilla_Token) { text, kind, loc };
    }

    String_View text = cdilla_lexer_cut_char(lexer);
    return (Cdilla_Token) { text, CDILLA_TOKEN_UNKNOWN, loc };
}
