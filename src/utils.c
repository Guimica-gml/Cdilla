#include "./utils.h"

size_t da_append_impl(void **items, Da_Header *header, const void *item, size_t item_size) {
    if (header->count >= header->capacity) {
        if (header->capacity == 0) {
            header->capacity = DA_INIT_CAP;
        } else {
            header->capacity *= 2;
        }
        *items = realloc(*items, header->capacity * item_size);
        assert(*items != NULL && "Error: not enough ram");
    }
    memcpy(((u8*)*items) + (item_size * header->count), item, item_size);
    header->count += 1;
    return header->count - 1;
}

void da_set_impl(void *items, Da_Header *header, const void *item, size_t item_size, size_t index) {
    assert(index < header->count);
    memcpy(((u8*)items) + (item_size * index), item, item_size);
}

void *da_get_impl(void *items, Da_Header *header, size_t item_size, size_t index) {
    assert(index < header->count);
    return ((u8*)items) + (item_size * index);
}

String_View sv_from_cstr(const char *cstr) {
    return (String_View) {
        .data = cstr,
        .count = strlen(cstr),
    };
}

String_View sv_from_sb(const String_Builder *sb) {
    return (String_View) {
        .data = sb->items,
        .count = da_count(sb),
    };
}

bool sv_equals(String_View a, String_View b) {
    if (a.count != b.count) return false;
    return memcmp(a.data, b.data, a.count) == 0;
}

void sb_add_sized_str(String_Builder *sb, const char *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        da_append(sb, data[i]);
    }
}

Errno read_file(const char *filepath, String_Builder *sb) {
    int result = 0;
    char *content = NULL;

    // NOTE(nic): opening in binary mode, because I don't want it to convert '\r\n' to '\n'
    //            this is because there's a problem with `file_size` and `read_size`
    //            being different because windows converts the break line characters
    //            which fails silently because technically no error happened
    //            but our code really thinks something went bad
    FILE *file = fopen(filepath, "rb");
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
