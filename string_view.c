#include <ctype.h>
#include <stdio.h>
#include "include/string_view.h"

String_View sv_from_cstr(const char* data){
    String_View sv;
    sv.data = data;
    sv.count = strlen(data);
    return sv;
}

String_View sv_with_size(const char* data,size_t n){
    String_View sv;
    sv.data = data;
    sv.count = n;
    return sv;

}
String_View sv_trim_left(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return sv_with_size(sv.data + i, sv.count - i);
}

String_View sv_trim_right(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return sv_with_size(sv.data, sv.count - i);
}

String_View sv_trim(String_View sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

String_View sv_split(String_View * sv, const char delim){
     size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    String_View result = sv_with_size(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }
    return result;
}


#ifdef TESTSV

int main(){
    char request_raw[] = "GET /tsoding/sv/blob/master/sv.h \r\n\
        Host: github.com \r\n\
        User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:151.0) Gecko/20100101 Firefox/151.0 \r\n\
        Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8 \r\n\
        Accept-Language: en-US,en;q=0.9 \r\n\
        Accept-Encoding: gzip, deflate, br, zstd \r\n\
        Referer: https://github.com/orgs/tsoding/repositories?q=string_view \r\n\
        Sec-GPC: 1 \r\n\
        Connection: keep-alive \r\n\
        Cookie: _gh_sess=qshalVuoUtixGqgSHBu3HlOSlHc4XrTVOvuwl4Y4InmhUkTTKbfWuuhKOa8Ld6XzGhqRcBWBSD7X6tur2lbVHWh9hGwTRGioEOVJ3tMONNq70Q%2BKziBURsB34OD5nX%2Fm4mxoAhA2DvuYEbcS44zokBJyNmnTEKTLCkcfaNHwBeYika5KaSMQcYmbedx7wuTXcv8UX1PnLKPIPODQ0lejbb%2Fc9mtBrZmqyzjh%2B8wecdsM%2F6zIvD3h0QnDcpjSTyGQxVVTVO8yIdyMrUwjOr7Fbg%3D%3D--U5CgRHE%2FnzkLyJD4--FXUmLMUvatdEkvRc3zglgg%3D%3D; _octo=GH1.1.939740711.1781081084; logged_in=no; cpu_bucket=lg; preferred_color_mode=dark; tz=Asia%2FKolkata \r\n\
        Upgrade-Insecure-Requests: 1 \r\n\
        Sec-Fetch-Dest: document \r\n\
        Sec-Fetch-Mode: navigate \r\n\
        Sec-Fetch-Site: same-origin \r\n\
        If-None-Match: W/\"984e808115500d78959cdcc1f7dc5aac\" \r\n\
        Priority: u=0, i\r\n\r\n\r\n";
    String_View sv = sv_from_cstr(request_raw);
    char delim = '\n';
    
    String_View res = sv_trim(sv_split(&sv,delim));
    String_View method,path,version;
    if(!sv_is_empty(res)){
        method = sv_trim(sv_split(&res, ' '));
        path = sv_trim(sv_split(&res, ' '));
        version = sv_trim(sv_split(&res, ' '));
        
    }

    res = sv_trim(sv_split(&sv,delim));
    printf("Method: "SV_Fmt" Path: "SV_Fmt" Version: "SV_Fmt"\n",SV_Arg(method),SV_Arg(path),SV_Arg(version)); 
    while(!sv_is_empty(res)){
        String_View key = sv_split(&res,':');
        if( !sv_is_empty(key) && !sv_is_empty(res) ){
            printf("Key: "SV_Fmt"\n", SV_Arg(sv_trim(key)));
            printf("Value: "SV_Fmt"\n", SV_Arg(sv_trim(res)));
        }
        res = sv_trim(sv_split(&sv,delim));
    }
}
#endif

