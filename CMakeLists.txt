cmake_minimum_required(VERSION 3.1)

project(b1mulator)

include_directories("engine/include")

set(CMAKE_CXX_STANDARD 11)
add_compile_options(-Wall -Werror -Wl,--no-undefined)

add_subdirectory("engine")
add_subdirectory("bin")

add_subdirectory("gui")
file(COPY "test/raw.data" DESTINATION "${CMAKE_BINARY_DIR}/bin")
