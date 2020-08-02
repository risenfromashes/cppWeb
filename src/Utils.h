#ifndef __CW_UTILS_H_
#define __CW_UTILS_H_

#include <cstring>
#include <cassert>
#include <string>
#include <chrono>
#include <type_traits>
#include <concepts>
#include "macro_foreach_def.h"
#include <iostream>
#include <vector>
namespace cW {

#define __INF__ static_cast<size_t>(-1)

template <typename T>
using clean_t = std::remove_cv_t<std::remove_pointer_t<T>>;

template <typename T>
constexpr bool is_ref_v = std::is_lvalue_reference_v<T> || std::is_rvalue_reference_v<T>;

template <typename, template <typename...> class>
constexpr bool is_specialization_v = false;

template <template <typename...> class T, typename... R>
constexpr bool is_specialization_v<T<R...>, T> = true;

template <typename T1, typename T2>
struct __mem_pointer_t {
    using _Class_T  = T1;
    using _Member_T = T2;
    constexpr __mem_pointer_t(T2 T1::*) {}
};

// clang-format off
template <auto _member_ptr>
    requires std::is_member_pointer_v<decltype(_member_ptr)> 
using member_pointer_class_t = decltype(__mem_pointer_t(_member_ptr))::_Class_T;
template <auto _member_ptr>
    requires std::is_member_pointer_v<decltype(_member_ptr)> 
using member_pointer_member_t = decltype(__mem_pointer_t(_member_ptr))::_Member_T;

// clang-format on

template <typename T, bool pointer, typename... R>
static inline auto createInstance(R... args)
{
    if constexpr (pointer)
        return new T(std::forward<R>(args)...);
    else
        return T(std::forward<R>(args)...);
}

template <typename T, bool pointer>
static inline auto createInstance()
{
    if constexpr (pointer)
        return new T();
    else
        return T();
}

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

void generate_macro_foreach_header(unsigned int max_item_count);

template <unsigned N>
struct FixedString {
    char buf[N + 1]{};
    constexpr FixedString(char const* s)
    {
        for (unsigned int i = 0; i != N; ++i)
            buf[i] = s[i];
    }
    constexpr operator char const*() const { return buf; }
};

template <unsigned N>
FixedString(char const (&)[N])->FixedString<N - 1>;

template <typename T1, typename T2>
struct offset_of_impl {
    static constexpr char           dummy[sizeof(T2)]{};
    static constexpr std::ptrdiff_t offset_of(T1 T2::*member)
    {
        T2* obj = (T2*)size_t(&dummy);
        return (size_t(&(obj->*member)) - size_t(obj));
    }
};

template <typename T1, typename T2>
constexpr std::ptrdiff_t offset_of(T1 T2::*member)
{
    return offset_of_impl<T1, T2>::offset_of(member);
}

#ifdef __cpp_nontype_template_parameter_class
// clang-format off
template <typename T, class _Class_T>
concept __PrintEntry = requires (T)
{ 
    {T::name} -> std::convertible_to<const char*>;
    T::print((_Class_T*)nullptr);
};
template <typename T>
concept PrintableVal = std::is_convertible_v<T, const char*> 
                        || (std::is_pointer_v<T> && 
                            requires(T a, std::ostream oss){ oss << (*a);} ||
                            requires(T a) { {a->toString()}->std::convertible_to<std::string>; }) ||
                           (!std::is_pointer_v<T> && 
                            requires(T a, std::ostream oss){ oss << a;} ||
                            requires(T a) { {a.toString()}->std::convertible_to<std::string>; });

// clang-format on

template <class _Class_T, PrintableVal _Member_T, FixedString _name, size_t _offset>
struct PrintEntry {
    static constexpr const char* name = _name;
    static void                  print(_Class_T* _this)
    {
        std::cout << name << ": ";
        if constexpr (std::is_convertible_v<_Member_T, const char*>) {
            std::cout << *((_Member_T*)(size_t(_this) + _offset)) << std::endl;
        }
        else if constexpr (std::is_pointer_v<_Member_T>) {
            if constexpr (requires(_Member_T a, std::ostream oss) { oss << (*a); })
                std::cout << *(*((_Member_T*)(size_t(_this) + _offset))) << std::endl;
            else
                std::cout << (*((_Member_T*)(size_t(_this) + _offset)))->toString() << std::endl;
        }
        else {
            if constexpr (requires(_Member_T a, std::ostream oss) { oss << a; })
                std::cout << *((_Member_T*)(size_t(_this) + _offset)) << std::endl;
            else
                std::cout << ((_Member_T*)(size_t(_this) + _offset))->toString() << std::endl;
        }
    }
};

template <class _Class_T, __PrintEntry<_Class_T>... _Entries>
struct PrintContext {
    static constexpr size_t entry_count = sizeof...(_Entries);
    template <size_t index, __PrintEntry<_Class_T> _entry, __PrintEntry<_Class_T>... _entries>
    static void _print(_Class_T* ptr)
    {
        _entry::print(ptr);
        if constexpr (index > 0) _print<index - 1, _entries...>(ptr);
    }
    static void print(_Class_T* ptr) { _print<entry_count - 1, _Entries...>(ptr); }
};

#define PRINTENTRY(type, mem) \
    , cW::PrintEntry<type, decltype(type::mem), #mem, cW::offset_of(&type::mem)>
#define PRINTABLE(type, ...)                                                                     \
    inline void print()                                                                          \
    {                                                                                            \
        cW::PrintContext<type CW_MACRO_FOR_EACH_EX(PRINTENTRY, type, __VA_ARGS__)>::print(this); \
    }
#else
#define PRINTABLE(type, ...) void print();
#endif

}; // namespace cW

#endif