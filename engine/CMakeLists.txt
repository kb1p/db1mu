file(GLOB sources_c "sources/*.c")
file(GLOB sources_cpp "sources/*.cpp")

set(sources ${sources_c} ${sources_cpp})

add_library(b1-eng SHARED ${sources})