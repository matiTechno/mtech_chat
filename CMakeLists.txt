cmake_minimum_required(VERSION 2.6)
project(asio)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(NOT CMAKE_BUILD_TYPE)
set (CMAKE_BUILD_TYPE Release)
endif()

if(APPLE)
    include_directories("/usr/local/include")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/local/lib")
endif()

link_libraries(-pthread)

add_subdirectory(src)

