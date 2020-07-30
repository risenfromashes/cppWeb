#include "Utils.h"
#include <iostream>
#include <fstream>

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

// reference:
// https://github.com/garbageslam/visit_struct/blob/master/include/visit_struct/visit_struct.hpp
void generate_macro_foreach_header(unsigned int max_item_count)
{
    std::ofstream def_file("src/macro_foreach_def.h");

    // def file begin
    def_file << "#ifndef __CW_MACRO_FOR_EACH__" << std::endl
             << "#define __CW_MACRO_FOR_EACH__" << std::endl
             << "#define CW_MACRO_FOREACH_MAX_ITEM_COUNT " << max_item_count << std::endl;
    for (int i = 0; i <= max_item_count; i++) {
        def_file << "#define CW_MACRO_FOR_EACH_" << i << "(f";
        for (int j = 1; j <= i; j++) {
            def_file << ",";
            def_file << "_" << j;
        }
        def_file << ") ";
        for (int j = 1; j <= i; j++) {
            def_file << "f(_" << j << ") ";
        }
        def_file << std::endl;

        def_file << "#define CW_MACRO_FOR_EACH_EX_" << i << "(f";
        for (int j = 1; j <= i; j++) {
            def_file << ",";
            def_file << "_" << j;
        }
        def_file << ",x) ";
        for (int j = 1; j <= i; j++) {
            def_file << "f(x,_" << j << ") ";
        }
        def_file << std::endl;
    }

    def_file << "#define CW_MACRO_NARGS_(";
    for (int i = 1; i <= max_item_count; i++) {
        def_file << "_" << i;
        if (i < max_item_count) def_file << ",";
    }
    def_file << ", n, ...) n" << std::endl;

    def_file << "#define CW_MACRO_NARGS(...) CW_MACRO_NARGS_(__VA_ARGS__,";
    for (int i = max_item_count; i >= 0; i--) {
        def_file << i;
        if (i > 0) def_file << ",";
    }
    def_file << ")" << std::endl;
    def_file << "#define CW_MACRO_CONCAT_(a,b) a##b" << std::endl
             << "#define CW_MACRO_CONCAT(a,b) CW_MACRO_CONCAT_(a,b)" << std::endl
             << "#define CW_MACRO_FOR_EACH(f,...) CW_MACRO_CONCAT(CW_MACRO_FOR_EACH_, "
                "CW_MACRO_NARGS(__VA_ARGS__))(f,__VA_ARGS__)"
             << std::endl
             << "#define CW_MACRO_FOR_EACH_EX(f,x,...) CW_MACRO_CONCAT(CW_MACRO_FOR_EACH_EX_, "
                "CW_MACRO_NARGS(__VA_ARGS__))(f,__VA_ARGS__,x)"
             << std::endl;
    def_file << "#endif";
    def_file.close();
    // def file end
}

}; // namespace cW