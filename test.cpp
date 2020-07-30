#include "src/Server.h"
#include <fstream>
int main()
{
    // std::ifstream file;
    // file.open("test_resources/index2.html", std::ios_base::binary);
    // file.seekg(0, file.end);
    // size_t size = file.tellg();
    // file.seekg(0, file.beg);
    // char* buf = (char*)malloc(size);
    // file.read(buf, size);
    cW::Server()
        .get("/",
             [](cW::HttpRequest* req, cW::HttpResponse* res) {
                 // res->setHeader("Server", "cW 0.0.1");
                 res->send("Hello!");
                 //  res->setHeader("Content-Type", "text/html");
                 //  res->setHeader("Content-Length", size);
                 //  res->onWritable(
                 //      [=](size_t offset) { res->write(buf + offset, size - offset, size); });
             })
        .listen(9001)
        .run(cW::MTMode::MULTIPLE_LISTENER);
}