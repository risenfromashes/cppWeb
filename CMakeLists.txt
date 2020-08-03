
cmake_minimum_required(VERSION 3.0.0)
project(cpp_web VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
include(CTest)
enable_testing()
set(POSITION_INDEPENDENT_CODE TRUE)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -pie -fconcepts")
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")



# find_package(OpenSSL REQUIRED)
# find_package(CURL REQUIRED)

# find_package(OpenSSL REQUIRED)
# find_library(ws2_lib Ws2_32.lib)
# find_package(ZLIB REQUIRED)

FILE(GLOB src_files "src/*.h" "src/*.cpp")

add_library(cppWeb SHARED ${src_files})


# add_executable(main main.cpp)
add_executable(main twitter_test.cpp)
target_link_libraries(main cppWeb)
target_include_directories(main PRIVATE 
# ${include_dir} 
)
# target_link_libraries(cpp_web PRIVATE ${ws2_lib} 
# # OpenSSL::Crypto CURL::libcurl
# )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)