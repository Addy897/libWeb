#include "main.h"
char * indexH(Request * req){
    return render_template("./app/index.html");
}
char * aboutH(Request * req){
    return  render_template("./app/about.html");
   
}
int main(int argc, char const *argv[])
{
    
    int init=initializeSocket();
    if(init==-1){
        cleanupRoutes();
        return -1;
    }
    char path[100];
   
    setStaticPath("./app");
    addRoute("/",indexH);
    addRoute("/about",aboutH);
    startServer("0.0.0.0",6969);
    cleanupRoutes();

}
