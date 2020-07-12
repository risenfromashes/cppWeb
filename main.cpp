#include "src/Server.h"

int main()
{
    cW::Server()
        .get("/",
             [](cW::HttpRequest* req, cW::HttpResponse* res) {
                 res->setHeader("x-server", "cW");
                 res->send("Hello!");
             })
        .listen(9001)
        .run();
}
