#ifndef MIMETYPES_H
#define MIMETYPES_H

static char* extensions[][2] ={
 {"gif", "image/gif" },
 {"txt", "text/plain" },
 {"jpg", "image/jpg" },
 {"jpeg","image/jpeg"},
 {"png", "image/png" },
 {"ico", "image/ico" },
 {"zip", "image/zip" },
 {"gz",  "image/gz"  },
 {"tar", "image/tar" },
 {"htm", "text/html" },
 {"html","text/html" },
 {"php", "text/html" },
 {"js", "text/javascript" },
 {"css","text/css"},
 {"json", "application/json"},
 {"pdf","application/pdf"},
 {"zip","application/octet-stream"},
 {"rar","application/octet-stream"}};

void getMiME(char * filename,char * mime);
#endif