#include "include/mimeTypes.h"
#include <string.h>

char *getMiME(char *filename) {
  char *token;
  char *ext;
  char *rest = strdup(filename);
  const char *deli = ".";
  while ((token = strtok_r(rest, deli, &rest)))
    ext = token;

  for (int i = 0; i < 17; i++) {
    if (strcmp(extensions[i][0], ext) == 0) {
      return extensions[i][1];
    }
  }
  return "text/plain";
}
