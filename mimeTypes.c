#include "include/mimeTypes.h"
#include <string.h>
#include <stdlib.h>

char *getMiME(char *filename) {
    char *original = strdup(filename);  
    char *rest = original;              
    char *token, *ext = NULL;
    const char *deli = ".";

    while ((token = strtok_r(rest, deli, &rest)))
        ext = token;

    char *mime = "text/plain";
    if (ext) {
        for (int i = 0; i < 17; i++) {
            if (strcmp(extensions[i][0], ext) == 0) {
                mime = extensions[i][1];
                break;
            }
        }
    }

    free(original);       
    return mime;      
}
