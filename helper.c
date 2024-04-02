#include "helper.h"
#include "globals.h"
#include <direct.h>

char STATIC_PATH[256];
void getStaticPath(char * path){
    
    strncpy(path,STATIC_PATH,strlen(STATIC_PATH));
    path[strlen(STATIC_PATH)]='\0';
}
void setStaticPath(char * path){
    int i=0;
    while(path[i]!='\0'){
        STATIC_PATH[i]=path[i];
        i++;
    }
    STATIC_PATH[i]='\0';
}
int hasExtension(char * filepath){
   
     char *query_start = strchr(filepath, '.');
     if (query_start != (void*)0) {
        return 1;
     }
     return 0;
}
int exists(char *fname)
{     
    if(hasExtension(fname)==0){
         
        return 0;
    }
    char WEB_PATH[256];
    getStaticPath(WEB_PATH);
    strcat(WEB_PATH,fname);
    FILE *file;
    file = fopen(WEB_PATH, "r");
    if (file)
    {
        fclose(file);
        return 1;
    }
    return 0;
}
