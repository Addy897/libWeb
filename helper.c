#include "include/helper.h"
char PUBLIC_DIR[256];
void getPublicDir(char *path) {

  strncpy(path, PUBLIC_DIR, strlen(PUBLIC_DIR));
  path[strlen(PUBLIC_DIR)] = '\0';
}
void setPublicDir(char *path) {
  int i = 0;
  while (path[i] != '\0') {
    PUBLIC_DIR[i] = path[i];
    i++;
  }
  PUBLIC_DIR[i] = '\0';
}
int hasExtension(char *filepath) {

  char *query_start = strchr(filepath, '.');
  if (query_start != (void *)0) {
    return 1;
  }
  return 0;
}
int exists(char *fname) {
  if (hasExtension(fname) == 0) {

    return 0;
  }
  char WEB_PATH[256];
  getPublicDir(WEB_PATH);
  strcat(WEB_PATH, fname);
  FILE *file;
  file = fopen(WEB_PATH, "r");
  if (file) {
    fclose(file);
    return 1;
  }
  return 0;
}
