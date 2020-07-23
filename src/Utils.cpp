#include "Utils.h"
#include <iostream>

namespace cW {

std::chrono::steady_clock::time_point Clock::start_time;

void Clock::start() { start_time = std::chrono::steady_clock::now(); }
void Clock::reset() { start(); }
void Clock::printElapsed(const std::string& message, bool reset)
{
    auto ms = (std::chrono::steady_clock::now() - start_time).count() / 1e6;
    std::cout << "(*) " << ms << " milliseconds elapsed. " << message << std::endl;
    if (reset) start();
}

char* url_decode(const char* str, size_t size)
{
    if (size == 0) size = strlen(str);
    size_t dest_len = size - count_char('%', str, size) * 2 + 1;
    char*  dest     = (char*)malloc(dest_len);
    size_t i, j;
    for (i = j = 0; j < dest_len - 1 && i < size; i++, j++) {
        if (str[i] == '%') {
            if (i + 2 < size) {
                dest[j] = char(from_hex(str[i + 1]) << 4 | from_hex(str[i + 2]));
                i += 2;
            }
        }
        else if (str[i] == '+')
            dest[j] = ' ';
        else
            dest[j] = str[i];
    }
    assert((j == dest_len - 1) && "Invalid url encoded string");
    dest[j] = '\0';
    return dest;
}

}; // namespace cW