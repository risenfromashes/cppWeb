cmake_minimum_required(VERSION 3.0.0)
project(cpp_web VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
include(CTest)
enable_testing()
set(POSITION_INDEPENDENT_CODE TRUE)

find_package(re2 CONFIG REQUIRED)
find_library(ws2_lib Ws2_32.lib)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

FILE(GLOB src_files "src/*.h" "src/*.cpp")

add_executable(cpp_web main.cpp ${src_files})
target_include_directories(cpp_web PRIVATE ${include_dir} ${RAPIDJSON_INCLUDE_DIRS})
target_link_libraries(cpp_web PRIVATE ${ws2_lib} ${pcre2} re2::re2 OpenSSL::Crypto CURL::libcurl)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
