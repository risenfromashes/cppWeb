#ifndef __CW_UTILS_H_
#define __CW_UTILS_H_

#include <cstring>
#include <cassert>
#include <string>
#include <chrono>

namespace cW {

#define __INF__ static_cast<size_t>(-1)

class Clock {
    static std::chrono::steady_clock::time_point start_time;

  public:
    static void start();
    static void reset();
    static void printElapsed(const std::string& message, bool reset = false);
};

// returns string of zeros and ones as binary data
template <typename T>
T* asBinary(const char* str)
{
    size_t size = strlen(str);
    assert(size % 8 == 0 && "Must contain full bytes.");
    uint8_t* ret = (uint8_t*)malloc(size / 8);

    for (size_t i = 0; i < size / 8; i++) {
        ret[i] = 0;
        for (int j = 0; j < 8; j++) {
            switch (str[8 * i + (8 - j - 1)]) {
                case '1': ret[i] |= (1 << j); break;
                case '0': break;
                default: throw std::exception("Can only contain zeros and ones");
            }
        }
    }
    return reinterpret_cast<T*>(ret);
}

template <typename T>
inline void reverseByteOrder(T* val, uint8_t size = 0)
{
    if (size == 0) size = sizeof(T);
    uint8_t* bytes = reinterpret_cast<uint8_t*>(val);
    for (int i = 0; i < size / 2; i++)
        std::swap(bytes[i], bytes[size - i - 1]);
}
}; // namespace cW

#endif