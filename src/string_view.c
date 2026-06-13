#include <ctype.h>
#include <stdio.h>
#include "string_view.h"


#define  is_digit(x) ('0'<= x && x <= '9')
#define  is_space(x) (x == ' ' || x == '\t' || x =='\r' || x == '\n' || x == '\v' || x == '\f')

StringView sv_from_cstr(const char* data){
    StringView sv;
    sv.data = data;
    sv.count = strlen(data);
    return sv;
}

StringView sv_from_size(const char* data,size_t n){
    StringView sv;
    sv.data = data;
    sv.count = n;
    return sv;

}

bool sv_eq(StringView left,StringView right){
     if(left.count != right.count){
        return false;
     }else if(left.count ==0){
        return true;
    }else{
        return memcmp(left.data,right.data,left.count) == 0;
     }
}


StringView sv_trim_left(StringView sv){
    size_t i = 0;
    while (i < sv.count && is_space(sv.data[i])) {
        i += 1;
    }

    return sv_from_size(sv.data + i, sv.count - i);
}

StringView sv_trim_right(StringView sv){
    size_t i = 0;
    while (i < sv.count && is_space(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return sv_from_size(sv.data, sv.count - i);
}

StringView sv_trim(StringView sv){
    return sv_trim_right(sv_trim_left(sv));
}

StringView sv_split(StringView * sv, const char delim){
    if (sv->count == 0 || !sv->data) return sv_from_size(sv->data, 0);

    void *ptr = memchr(sv->data, delim, sv->count);
    size_t i = ptr ? (size_t)((char*)ptr - sv->data) : sv->count;

    StringView result = sv_from_size(sv->data, i);

    if (i < sv->count) {
        sv->data  += i + 1;
        sv->count -= i + 1;
    } else {
        sv->data += i;
        sv->count = 0;
    }    
    return result;
}

StringView sv_split_sv(StringView * sv, StringView delim){
    if (delim.count == 0 || sv->count == 0 || !sv->data) return sv_from_size(sv->data, 0);
    
    size_t i = 0;
    while (i + delim.count <= sv->count) {
        void *ptr = memchr(sv->data + i, delim.data[0], sv->count - i - delim.count + 1);
        if (!ptr) break;
        
        i = (size_t)((char*)ptr - sv->data);
        if (memcmp(sv->data + i, delim.data, delim.count) == 0) {
            StringView result = sv_from_size(sv->data, i);
            sv->data  += i + delim.count;
            sv->count -= i + delim.count;
            return result;
        }
        i++;
    }

    StringView result = sv_from_size(sv->data, sv->count);
    sv->data += sv->count;
    sv->count = 0;

    return result;
}

bool sv_eq_ignorecase(StringView left,StringView right){
   if(left.count != right.count){
        return false;
    } 
    char l,r;
    for(size_t i = 0;i<left.count;i++){
        if(left.data[i] >= 'A' && left.data[i] <= 'Z'){
            l = left.data[i] + 32;
        }else{
            l = left.data[i];
        }

        if(right.data[i] >= 'A' && right.data[i] <= 'Z'){
            r = right.data[i] + 32;
        }else{
            r = right.data[i];
        }
        if(l != r) return false; 
    } 
    return true;
}
int sv_to_int(StringView sv){
    int res = 0;
    for(int i =0;i<sv.count && is_digit(sv.data[i]);i++){
            res = res * 10 + (sv.data[i] - '0');
    }
    return res;
        
}

StringView sv_to_lowercase(StringView sv){
    if (!sv.data) return sv;
    for (size_t i = 0; i<sv.count; i++) {
        if(sv.data[i] >= 'A' && sv.data[i] <= 'Z') {
            sv.data[i] += 32;         
        }
    }
    return sv; 
}
