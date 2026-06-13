#ifndef HELPER_H
#define HELPER_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
 

void print_error(char*);
int exists(char *filename);
void getPublicDir(char *path);
void setPublicDir(char *path);
char *trim(char *str);
char* strsplit(char * source, char* str,char** saved);
void toLowerCase(char *str);
#endif
