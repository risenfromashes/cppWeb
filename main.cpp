
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <queue>
#include <list>
#include <map>
#include "src/Json.h"

struct A {
    int x, y;
    JSONABLE(A, x, y);
};

struct Point {
    const char* name;
    double      x, y;
    JSONABLE(Point, x, y, name);
};

class P {
    A                  a{10, 9};
    int                x, y;
    const A            nums[2][2] = {{{9, 10}, {8, 9}}, {{47, 11}, {7, 49}}};
    std::vector<Point> points;
    std::string        name;

  private:
    bool active = true;
    int* xp     = &x;

    JSONABLE(P, a, points, nums, x, y, xp, name, active);
};

int main()
{
    // std::ofstream file("test_json.json");
    // P             p;
    // p.x    = 2;
    // p.y    = 9;
    // p.name = "JSON object v3";
    // p.points.push_back({"good point", 12.3, 445.0});
    // p.points.push_back({"bad point", 6.9, 6.9});
    // p.writeJson(file);
    // std::ifstream file("test_resources/twitter.json");
    // std::ifstream file("test_resources/test_json.json");
    // file.seekg(0, file.end);
    // size_t fileSize = file.tellg();
    // char*  buf      = (char*)malloc(fileSize);
    // file.seekg(0, file.beg);
    // file.read(buf, fileSize);
    // cW::Clock::start();
    // auto rootNode = cW::JsonParser::parse(buf);
    // cW::Clock::printElapsed("Parsed json");
    // // std::cout <<
    // // auto v = rootNode->get<std::vector<int>>("/statuses/1/user/entities/url/urls/0/indices");
    // auto v = rootNode->get<std::vector<std::vector<int>>>("/nums");
    // for (auto j : v) {
    //     for (auto i : j)
    //         std::cout << i << " ";
    //     std::cout << std::endl;
    // }
    //<< std::endl;
}
