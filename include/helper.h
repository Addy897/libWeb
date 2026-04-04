#ifndef HELPER_H
#define HELPER_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
int exists(char *filename);
void getPublicDir(char *path);
void setPublicDir(char *path);
char *trim(char *str);
#endif
