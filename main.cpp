
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <queue>
#include <list>
#include <map>
#include "src/Server.h"

int main()
{
    std::ifstream file("test_resources/index.html");
    file.seekg(0, file.end);
    size_t fileSize = file.tellg();
    char*  buffer   = (char*)malloc(fileSize);
    file.seekg(0, file.beg);
    file.read(buffer, fileSize);
    cW::Server()
        .get("/{s:name}",
             [buffer](cW::HttpRequest* req, cW::HttpResponse* res) { res->send(buffer); })
        .listen(8001)
        .run();
}
