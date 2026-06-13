#include "helper.h"
#include "globals.h"
#include <sys/stat.h>
#include<stdlib.h>
 
char PUBLIC_DIR[PATH_MAX] = {0};
size_t PUBLIC_DIR_LEN = 0;


void setPublicDir(char *path) {
  char* r = realpath(path,PUBLIC_DIR);
  PUBLIC_DIR_LEN = strlen(PUBLIC_DIR);
}
int hasExtension(char *filepath) {

  char *query_start = strchr(filepath, '.');
  if (query_start != (void *)0) {
    return 1;
  }
  return 0;
}
int exists(char *fname) {
   struct stat path_stat;
    if (stat(fname, &path_stat) == 0) {
        return S_ISREG(path_stat.st_mode);
    }
    return 0;

}

void toLowerCase(char *str) {
    for (size_t i = 0; str[i]; i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;         
        }
    }

}
char *trim(char *str)
{
  char *end;

  while(isspace((unsigned char)*str)) str++;

  if(*str == 0) return str;

  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;
  end[1] = '\0';
  return str;
}

char* strsplit(char * source, char* str,char** saved){
    if(!str || !source ) return NULL;
    int size = strlen(str); 
    char* line = strstr(source,str);
 
    if(line == NULL) return NULL;
    *saved = line + size;
    
    *line = '\0';
    return source;
}

