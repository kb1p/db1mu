option(CPU_TRACE "Enable / disable tracing of currently executed CPU command" OFF)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.l.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-function")

if(CPU_TRACE)
    add_definitions(-DENABLE_CPU_TRACE)
endif()

set(sources "sources/Cartridge.cpp"
            "sources/cpu6502.cpp"
            "sources/gamepad.cpp"
            "sources/log.cpp"
            "sources/PPU.cpp"
            "sources/bus.cpp"
            "sources/common.cpp"
            "sources/loader.cpp"
            "sources/mappers.cpp")

if(BUILD_DEBUGGER)
    FIND_PACKAGE(BISON REQUIRED)
    FIND_PACKAGE(FLEX REQUIRED)

    BISON_TARGET(dbg_cmd_parser sources/dbg_cmd.y ${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.y.cpp)
    FLEX_TARGET(dbg_cmd_lexer sources/dbg_cmd.l  ${CMAKE_CURRENT_BINARY_DIR}/dbg_cmd.l.cpp)
    ADD_FLEX_BISON_DEPENDENCY(dbg_cmd_lexer dbg_cmd_parser)

    set(sources ${sources}
                "sources/debugger.cpp"
                ${BISON_dbg_cmd_parser_OUTPUTS}
                ${FLEX_dbg_cmd_lexer_OUTPUTS})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

add_library(b1-eng SHARED ${sources})

if(BUILD_DEBUGGER)
    target_link_libraries(b1-eng l pthread)
endif()
