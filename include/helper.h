#ifndef HELPER_H
#define HELPER_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "compat.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
extern char PUBLIC_DIR[PATH_MAX];
extern size_t PUBLIC_DIR_LEN;
 

void print_error(char*);
int exists(char *filename);
void getPublicDir(char *path);
void setPublicDir(char *path);
char *trim(char *str);
char* strsplit(char * source, char* str,char** saved);
void toLowerCase(char *str);
#endif
