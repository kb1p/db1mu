file(GLOB sources_c "sources/*.c")
file(GLOB sources_cpp "sources/*.cpp")

set(sources ${sources_c} ${sources_cpp})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(b1-eng SHARED ${sources})