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

#define DA_INIT_CAP 512
#define da_append(da, item)                                             \
    do {                                                                \
        if ((da)->capacity <= (da)->count) {                            \
            (da)->capacity = ((da)->capacity == 0) ? DA_INIT_CAP : (da)->capacity * 2; \
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
        }                                                               \
        (da)->items[(da)->count++] = (item);                            \
    } while(0);

#define da_free(da)                             \
    do {                                        \
        free((da)->items);                      \
        (da)->items = NULL;                     \
        (da)->count = 0;                        \
        (da)->capacity = 0;                     \
    } while (0);

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

typedef struct {
    const char *data;
    size_t count;
} String_View;

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
