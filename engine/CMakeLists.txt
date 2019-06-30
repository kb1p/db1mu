file(GLOB sources_c "sources/*.c")
file(GLOB sources_cpp "sources/*.cpp")

FIND_PACKAGE(BISON REQUIRED)
FIND_PACKAGE(FLEX REQUIRED)

option(CPU_TRACE "Enable / disable tracing of currently executed CPU command" ON)

BISON_TARGET(dbg_cmd_parser sources/dbg_cmd.y ${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.y.cpp)
FLEX_TARGET(dbg_cmd_lexer sources/dbg_cmd.l  ${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.l.cpp)
ADD_FLEX_BISON_DEPENDENCY(dbg_cmd_lexer dbg_cmd_parser)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.l.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-function")

if(CPU_TRACE)
    add_definitions(-DENABLE_CPU_TRACE)
endif()

set(sources
        ${sources_c}
        ${sources_cpp}
        ${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.y.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.l.cpp
        )

add_library(b1-eng SHARED ${sources})

target_link_libraries(b1-eng l pthread)
