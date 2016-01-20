cmake_minimum_required(VERSION 2.8)

project(b1mulator)

include_directories("engine/include")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11 -Wl,--no-undefined")

add_subdirectory("engine")
add_subdirectory("bin")

#add_subdirectory("gui")
