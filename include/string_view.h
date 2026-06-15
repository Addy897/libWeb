#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    const char *data;
    size_t count;
} StringView;

#define SV_NULL ((StringView){NULL, 0})
#define sv_from_cltr(lit) ((StringView){(const char *)(lit), sizeof(lit) - 1})

#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int)(sv).count, (sv).data

#define sv_is_empty(sv) ((sv).count == 0)

static inline StringView sv_from_cstr(const char *data)
{
    return (StringView){ data, data ? strlen(data) : 0 };
}

static inline StringView sv_from_size(const char *data, size_t n)
{
    return (StringView){ data, n };
}

static inline bool sv_eq(StringView a, StringView b)
{
    if (a.count != b.count) return false;
    if (a.count == 0) return true;
    return memcmp(a.data, b.data, a.count) == 0;
}

static inline char sv_to_lower_char(char c)
{
    return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}

static inline bool sv_eq_ignorecase(StringView a, StringView b)
{
    if (a.count != b.count) return false;

    for (size_t i = 0; i < a.count; i++) {
        if (sv_to_lower_char(a.data[i]) != sv_to_lower_char(b.data[i])) {
            return false;
        }
    }
    return true;
}

static inline bool is_space(char c)
{
    return (c == ' ' || c == '\t' || c == '\r' ||
            c == '\n' || c == '\v' || c == '\f');
}

static inline StringView sv_trim_left(StringView sv)
{
    size_t i = 0;
    while (i < sv.count && is_space(sv.data[i])) {
        i++;
    }
    return sv_from_size(sv.data + i, sv.count - i);
}

static inline StringView sv_trim_right(StringView sv)
{
    size_t i = sv.count;
    while (i > 0 && is_space(sv.data[i - 1])) {
        i--;
    }
    return sv_from_size(sv.data, i);
}

static inline StringView sv_trim(StringView sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

static inline StringView sv_split(StringView *sv, char delim)
{
    if (!sv->data || sv->count == 0) {
        return SV_NULL;
    }

    const char *start = sv->data;
    const void *p = memchr(start, delim, sv->count);

    size_t idx = p ? (size_t)((const char *)p - start) : sv->count;

    StringView out = { start, idx };

    if (idx < sv->count) {
        sv->data  += idx + 1;
        sv->count -= idx + 1;
    } else {
        sv->data += idx;
        sv->count = 0;
    }

    return out;
}

static inline StringView sv_split_sv(StringView *sv, StringView delim)
{
    if (!sv->data || sv->count == 0 || delim.count == 0) {
        return SV_NULL;
    }

    for (size_t i = 0; i + delim.count <= sv->count; i++) {
        if (sv->data[i] == delim.data[0]) {
            if (memcmp(sv->data + i, delim.data, delim.count) == 0) {
                StringView out = { sv->data, i };
                sv->data  += i + delim.count;
                sv->count -= i + delim.count;
                return out;
            }
        }
    }

    StringView out = *sv;
    sv->data += sv->count;
    sv->count = 0;
    return out;
}

static inline int sv_to_int(StringView sv)
{
    int res = 0;
    for (size_t i = 0; i < sv.count; i++) {
        char c = sv.data[i];
        if (c < '0' || c > '9') break;
        res = res * 10 + (c - '0');
    }
    return res;
}

#endif
