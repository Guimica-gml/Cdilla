#include "./utils.h"

String_View sv_from_cstr(const char *cstr) {
    return (String_View) {
        .data = cstr,
        .count = strlen(cstr),
    };
}

String_View sv_from_sb(const String_Builder *sb) {
    return (String_View) {
        .data = sb->items,
        .count = sb->count,
    };
}

char sv_chop_char(String_View *sv) {
    assert(sv->count > 0);
    char ch = sv->data[0];
    sv->data += sizeof(char);
    sv->count -= 1;
    return ch;
}

String_View sv_chop_by_delim(String_View *sv, char delim) {
    String_View chopped = {0};
    chopped.data = sv->data;
    while (sv->count > 0 && sv->data[0] != delim) {
        sv_chop_char(sv);
        chopped.count += 1;
    }
    if (sv->count > 0) {
        sv_chop_char(sv);
    }
    return chopped;
}

void sv_trim_whitespace(String_View *sv) {
    if (sv->count == 0) return;
    while (sv->count > 0 && isspace(sv->data[0])) {
        sv_chop_char(sv);
    }
    while (sv->count > 0 && isspace(sv->data[sv->count - 1])) {
        sv->count -= 1;
    }
}

bool sv_eq(String_View a, String_View b) {
    if (a.count != b.count) return false;
    return memcmp(a.data, b.data, a.count) == 0;
}

bool sv_starts_with(String_View sv, String_View prefix) {
    if (sv.count < prefix.count) return false;
    return memcmp(sv.data, prefix.data, prefix.count) == 0;
}

void sb_add_sized_str(String_Builder *sb, const char *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        da_append(sb, data[i]);
    }
}

void sb_free(String_Builder *sb) {
    da_free(sb);
}

Errno read_file(const char *filepath, String_Builder *sb) {
    int result = 0;
    char *content = NULL;

    FILE *file = fopen(filepath, "r");
    if (file == NULL) defer_return(errno);
    if (fseek(file, 0, SEEK_END) != 0) defer_return(errno);
    long file_size = ftell(file);
    if (file_size < 0) defer_return(errno);
    rewind(file);

    content = malloc(file_size);
    assert(content != NULL && "Error: not enough ram");

    long read_size = fread(content, sizeof(char), file_size, file);
    if (read_size != file_size) defer_return(errno);
    sb_add_sized_str(sb, content, file_size);

defer:
    if (content) free(content);
    if (file) fclose(file);
    return result;
}
