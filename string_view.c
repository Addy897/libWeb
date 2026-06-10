#include <ctype.h>
#include <stdio.h>
#include "include/string_view.h"
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
     }else{
        return memcmp(left.data,right.data,left.count) == 0;
     }
}


StringView sv_trim_left(StringView sv){
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return sv_from_size(sv.data + i, sv.count - i);
}

StringView sv_trim_right(StringView sv){
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return sv_from_size(sv.data, sv.count - i);
}

StringView sv_trim(StringView sv){
    return sv_trim_right(sv_trim_left(sv));
}

StringView sv_split(StringView * sv, const char delim){
     size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }
    StringView result = sv_from_size(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    }else{

        sv->count -=i;
        sv->data +=i;
    }    
    return result;
}

StringView sv_split_sv(StringView * sv, StringView delim){
    StringView window = sv_from_size(sv->data, delim.count);
    size_t i = 0;
    while (i + delim.count < sv->count
        && !(sv_eq(window, delim)))
    {
        i++;
        window.data++;
    }

    StringView result = sv_from_size(sv->data, i);

    if (i + delim.count == sv->count) {
        result.count += delim.count;
    }

    sv->data  += i + delim.count;
    sv->count -= i + delim.count;

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
    for(int i =0;i<sv.count && isdigit(sv.data[i]);i++){
            res = res * 10 + (sv.data[i] - '0');
    }
    return 0;
        
}


