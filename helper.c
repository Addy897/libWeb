#include "include/helper.h"
#include <sys/stat.h>
#include<stdlib.h>
char PUBLIC_DIR[PATH_MAX];
void getPublicDir(char *path) {

  strncpy(path, PUBLIC_DIR, strlen(PUBLIC_DIR));
  path[strlen(PUBLIC_DIR)] = '\0';
}
void setPublicDir(char *path) {
    char* r = realpath(path,PUBLIC_DIR);
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
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
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
