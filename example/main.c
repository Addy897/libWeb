#include "main.h"
char * indexH(Request * req){
    return "<html><head><title>C</title></head><body>Hello From C</body></html>";
}

int main(int argc, char const *argv[])
{
    
    int init=initializeSocket();
    if(init==-1){
        cleanupRoutes();
        return -1;
    }
    char path[100];
   
    addRoute("/",indexH);
    startServer("0.0.0.0",6969);
    cleanupRoutes();

}
