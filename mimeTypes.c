#include "mimeTypes.h"
#include <string.h>

void getMiME(char * filename,char * mime){
    char* token;
    char* ext;
    char* rest=strdup(filename);
    const char * deli= ".";
    while ((token = strtok_r(rest, deli, &rest)))
        ext=token;
        
    for(int i=0;i<17;i++){
        if(strcmp(extensions[i][0],ext)==0){
           
            strncpy(mime,extensions[i][1],strlen(extensions[i][1]));
           
            return;
        }
    }
    strncpy(mime,"text/plain",11);
   
}
