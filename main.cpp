
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
    JSONABLE;
    int x, y;
};
SET_JSONABLE(A, x, y);

struct Point {
    JSONABLE;
    const char* name;
    double      x, y;
};
SET_JSONABLE(Point, x, y, name);

class P {
    JSONABLE;
    A                  a{10, 9};
    int                x, y;
    const A            nums[2][2] = {{{9, 10}, {8, 9}}, {{47, 11}, {7, 49}}};
    std::vector<Point> points;
    std::string        name;

  private:
    bool active = true;
    int* xp     = &x;
};
SET_JSONABLE(P, a, nums, points, x, y, xp, name, active);

template <typename T>
void f()
{
    T a;
    a[0] = 1;
    std::cout << a[0] << std::endl;
}

struct C {
    const int         a;
    const int         b;
    const std::string str;
    C(int a, int b) : a(a), b(b), str("Fuck you!") {}
    C(const C&) = delete;
};

C get() { return C{1, 2}; }

int main()
{
    C c{11, 2};
    new ((void*)&c.a) int(6);
    new ((void*)&c.str) std::string("Love You!");
    // C c = get();
    std::cout << c.a << " " << c.b << " " << c.str << std::endl;
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
