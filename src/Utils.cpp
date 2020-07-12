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

}; // namespace cW