#ifndef HELPER_H
#define HELPER_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


int exists(char *filename);
void getPublicDir(char *path);
void setPublicDir(char *path);
char *trim(char *str);


void toLowerCase(char *str);
#endif
