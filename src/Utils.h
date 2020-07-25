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

constexpr size_t count_char(char c, const char* str, size_t size = std::string::npos)
{
    size_t res = 0;
    for (size_t i = 0; i < size; i++) {
        if (str[i] == '\0') break;
        if (str[i] == c) res++;
    }
    return res;
}

constexpr bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }

constexpr char to_lower(char ch) { return ch | 32; }

inline std::string to_lower(const std::string& str)
{
    std::string ret;
    ret.resize(str.size());
    for (size_t i = 0; i < str.size(); i++)
        ret[i] = to_lower(str[i]);
    return ret;
}

constexpr uint16_t from_hex(char hexCh)
{
    return is_digit(hexCh) ? hexCh - '0' : to_lower(hexCh) - 'a' + 10;
}

template <bool rightLowerCase = false>
constexpr bool ci_match(const char* a, size_t a_size, const char* b, size_t b_size)
{
    if (a_size != b_size) return false;
    if constexpr (rightLowerCase) {
        for (size_t i = 0; i < a_size; i++)
            if (to_lower(a[i]) != b[i]) return false;
    }
    else {
        for (size_t i = 0; i < a_size; i++)
            if (to_lower(a[i]) != to_lower(b[i])) return false;
    }
    return true;
}

template <bool rightLowerCase = false>
constexpr bool ci_match(const std::string& a, const std::string& b)
{
    return ci_match<rightLowerCase>(a.data(), a.size(), b.data(), b.size());
}

template <bool rightLowerCase = false>
constexpr bool ci_match(const std::string_view& a, const std::string_view& b)
{
    return ci_match<rightLowerCase>(a.data(), a.size(), b.data(), b.size());
}

inline bool contains(const std::string& str, const char* pattern)
{
    return str.find(pattern) != std::string::npos;
}
inline bool contains(const std::string_view& str, const char* pattern)
{
    return str.find(pattern) != std::string::npos;
}

template <bool rightLowerCase = false>
inline size_t ci_find(const char* a, size_t a_size, const char* b, size_t b_size)
{
    for (size_t i = 0; i < a_size; i++) {
        bool match = true;
        if constexpr (rightLowerCase) {
            for (size_t j = 0; j < b_size && i + j < a_size; j++)
                if (to_lower(a[i + j]) != b[j]) {
                    match = false;
                    break;
                }
            if (match) return i;
        }
        else {
            for (size_t j = 0; j < b_size && i + j < a_size; j++)
                if (to_lower(a[i + j]) != to_lower(b[j])) {
                    match = false;
                    break;
                }
            if (match) return i;
        }
    }
    return __INF__;
}

template <bool rightLowerCase = false>
inline size_t ci_find(const std::string& str, const char* pattern)
{
    return ci_find<rightLowerCase>(str.data(), str.size(), pattern, strlen(pattern));
}

template <bool rightLowerCase = false>
inline size_t ci_find(const std::string_view& str, const char* pattern)
{
    return ci_find<rightLowerCase>(str.data(), str.size(), pattern, strlen(pattern));
}

char* url_decode(const char* str, size_t size = 0);

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