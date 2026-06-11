#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>


typedef struct {
    char * data;
    size_t count;
}StringView;

#define SV_NULL sv_from_size(NULL,0)
#define sv_from_cltr(lit) sv_from_size(lit,sizeof(lit)-1)
#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int) sv.count, sv.data
#define sv_is_empty(sv) (sv.count < 1 || *(sv.data) == '\0')    


StringView sv_from_cstr(const char* data);
StringView sv_from_size(const char* data, size_t n);
StringView sv_trim_left(StringView sv);
StringView sv_trim_right(StringView sv);
StringView sv_trim(StringView sv);
StringView sv_split(StringView* sv, char delim);
StringView sv_split_sv(StringView* sv, StringView delim);
StringView sv_to_lowercase(StringView sv);

int  sv_to_int(StringView sv);

bool sv_eq(StringView left,StringView right);
bool sv_eq_ignorecase(StringView left,StringView right);

#endif

