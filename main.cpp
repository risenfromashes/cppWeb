#include "src/Server.h"
#include "src/Utils.h"
#include <fstream>

int main()
{
    std::ifstream file;
    file.open("test_resources/index.html", std::ios::binary);
    file.seekg(0, file.end);
    size_t fileSize = file.tellg();
    file.seekg(0, file.beg);
    char* buffer = (char*)malloc(fileSize);
    file.read(buffer, fileSize);
    cW::Server()
        .get("/",
             [&](cW::HttpRequest* req, cW::HttpResponse* res) {
                 res->setHeader("x-server", "cW");
                 res->onWritable([res, &buffer, &fileSize](size_t offset) {
                     std::cout << "Response in handler: " << res << std::endl;
                     res->write(std::string_view(buffer + offset, fileSize), fileSize);
                 });
                 // res->send("Hello!");
             })
        .listen(9001)
        .run();
}
