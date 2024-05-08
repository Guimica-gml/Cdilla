#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <ctype.h>

#define array_len(arr) (sizeof(arr) / sizeof((arr)[0]))

#define defer_return(res) do { result = (res); goto defer; } while (0)

#define SV(cstr) { .data = (cstr), .count = sizeof(cstr) - 1 }

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int) (sv).count, (sv).data

#define BYTE_FMT "%c%c%c%c%c%c%c%c"
#define BYTE_ARG(byte)           \
    ((byte) & 0x80 ? '1' : '0'), \
    ((byte) & 0x40 ? '1' : '0'), \
    ((byte) & 0x20 ? '1' : '0'), \
    ((byte) & 0x10 ? '1' : '0'), \
    ((byte) & 0x08 ? '1' : '0'), \
    ((byte) & 0x04 ? '1' : '0'), \
    ((byte) & 0x02 ? '1' : '0'), \
    ((byte) & 0x01 ? '1' : '0')

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint16_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef int Errno;

#define DA_INIT_CAP 1024

typedef struct {
    size_t count;
    size_t capacity;
} Da_Header;

#define Da_Type(type)                           \
    struct {                                    \
        Da_Header header;                       \
        type *items;                            \
    }

#define da_append(da, item)                     \
    da_append_impl(                             \
        ((void**) (&(da)->items)),              \
        (&(da)->header),                        \
        &item,                                  \
        sizeof(*(da)->items))

#define da_set(da, index, item)                 \
    da_set_impl(                                \
        (da)->items,                            \
        (&(da)->header),                        \
        &item),                                 \
        sizeof(*(da)->items),                   \
        index)

#define da_get(da, index)                       \
    da_get_impl(                                \
        (da)->items,                            \
        (&(da)->header),                        \
        sizeof(*(da)->items),                   \
        index)

#define da_free(da)                             \
    do {                                        \
        free((da)->items);                      \
        (da)->items = NULL;                     \
        (da)->header.count = 0;                 \
        (da)->header.capacity = 0;              \
    } while(0);

#define da_count(da) (da)->header.count
#define da_cap(da) (da)->header.capacity

typedef Da_Type(char) String_Builder;

typedef struct {
    const char *data;
    size_t count;
} String_View;

size_t da_append_impl(void **items, Da_Header *header, const void *item, size_t item_size);
void da_set_impl(void *items, Da_Header *header, const void *item, size_t item_size, size_t index);
void *da_get_impl(void *items, Da_Header *header, size_t item_size, size_t index);

String_View sv_from_cstr(const char *cstr);
String_View sv_from_sb(const String_Builder *sb);
char sv_chop_char(String_View *sv);
String_View sv_chop_by_delim(String_View *sv, char delim);
void sv_trim_whitespace(String_View *sv);
bool sv_eq(String_View a, String_View b);
bool sv_starts_with(String_View sv, String_View prefix);

void sb_add_sized_str(String_Builder *sb, const char *data, size_t size);
void sb_free(String_Builder *sb);

Errno read_file(const char *filepath, String_Builder *sb);

#endif // UTILS_H_
