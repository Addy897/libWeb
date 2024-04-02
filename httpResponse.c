#include "httpResponse.h"
void HTTPResponse(char *body, char *resp, int size, char *type)
{
    sprintf(resp, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\n\n%s", type, size, body);
}
void HTMLResponse(char *response, char *body)
{
    sprintf(response, "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s", strlen(body), body);
}
char * render_template(char * pathname){
    FILE *file = fopen(pathname, "r"); 
    if (file != NULL) {
        
        char* html_content = (char*)malloc(1); 
        if (html_content == NULL) {
            fclose(file);
            return NULL;
        }
        html_content[0] = '\0';

        char buffer[1024];
        size_t totalBytesRead = 0;
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            char* temp = realloc(html_content, totalBytesRead + bytesRead + 1);
            if (temp == NULL) {
                free(html_content);
                fclose(file);
                return NULL;
            }
            html_content = temp;
            memcpy(html_content + totalBytesRead, buffer, bytesRead);
            totalBytesRead += bytesRead;
           
             
        }
        html_content[totalBytesRead] = '\0';
        fclose(file); 
        
        return html_content;
    }
}
int sendFile(SOCKET client, char *filepath)
{
    char WEB_PATH[256];
    getStaticPath(WEB_PATH);
    strncat(WEB_PATH, filepath, strlen(filepath));
   
    char data[1024];
    FILE* fptr = fopen(WEB_PATH, "r");
    if (!fptr)
    {
        printf("Error no such file: %s\n", WEB_PATH);
        return 0;
    }
    fseek(fptr, 0L, SEEK_END);
    int size = ftell(fptr);
    fclose(fptr);
    char mime[30];
    getMiME(filepath, mime);
    char file[1024];
    HTTPResponse("", file, size, mime);
    send(client, file, strlen(file), 0);
    int fp = open(WEB_PATH, O_RDONLY | O_BINARY);
    if (!fp)
    {
        printf("Error no such file: %s\n", WEB_PATH);
        return 0;
    }
    size_t bytes_read;
    size_t total_sent = 0;
    char buffer[1024];
    while ((bytes_read = read(fp, buffer, 1024)) > 0)
    {
        size_t bytes_sent = send(client, buffer, bytes_read, 0);
        if (bytes_sent == -1)
        {
            perror("Error sending data");
           return 0;
        }
        total_sent += bytes_sent;
        if (total_sent >= size)
        {
            break;
        }
    }
    close(fp);

    return 1;
}
