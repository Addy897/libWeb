#include <stdio.h>
#include <string.h>



typedef struct {
    char * data;
    size_t count;
}String_View;


#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int) sv.count, sv.data
#define sv_is_empty(sv) (sv.count < 1 || *(sv.data) == '\0')    

String_View sv_from_cstr(const char* data);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);



