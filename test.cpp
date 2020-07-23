#include "src/Server.h"

int main()
{
    cW::Server().get("/", [](cW::HttpRequest *req, cW::HttpResponse *res) {
                    res->send("Hello!");
                })
        .listen(9001)
        .run();
}