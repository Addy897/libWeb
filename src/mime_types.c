#include <mime_types.h>
#include <string.h>
#include <stdlib.h>
char *extensions[][2] = {{"gif", "image/gif"},
                                {"txt", "text/plain"},
                                {"jpg", "image/jpg"},
                                {"jpeg", "image/jpeg"},
                                {"png", "image/png"},
                                {"ico", "image/ico"},
                                {"zip", "image/zip"},
                                {"gz", "image/gz"},
                                {"tar", "image/tar"},
                                {"htm", "text/html"},
                                {"html", "text/html"},
                                {"php", "text/html"},
                                {"js", "text/javascript"},
                                {"css", "text/css"},
                                {"json", "application/json"},
                                {"pdf", "application/pdf"},
                                {"wasm", "application/wasm"},
                                {"zip", "application/octet-stream"},
                                {"rar", "application/octet-stream"}};


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
