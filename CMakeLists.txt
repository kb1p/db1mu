cmake_minimum_required(VERSION 3.1)

project(b1mulator)

option(BUILD_DEBUGGER "Build command line-based debugger" OFF)
set(FRONTEND_TYPE "SDL" CACHE STRING "Which frontend to use [SDL, QT, NONE]")
set(RENDERER_TYPE "GLES" CACHE STRING "Which renderer to use [GLES, Vulkan]")

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    "engine/include")

set(CMAKE_CXX_STANDARD 11)
#add_compile_options(-Wall -Werror -Wl,--no-undefined)

add_subdirectory("engine")

if(BUILD_DEBUGGER)
    add_subdirectory("bin")
endif()

if(RENDERER_TYPE STREQUAL "Vulkan")
    message(">>> Using Vulkan renderer")

    set(USE_VULKAN ON)
    add_definitions(-DUSE_VULKAN)

    # Create target to compile GLSL shaders to Spir-V binaries
    find_program(GLSLANG_VALIDATOR NAMES "glslangValidator")
    if (NOT GLSLANG_VALIDATOR)
        message(FATAL_ERROR "glslangValidator binary not found!")
    endif()
    message(STATUS "Found glslangValidator at ${GLSLANG_VALIDATOR}")

    set(shader_vertex_glsl "${PROJECT_SOURCE_DIR}/gui/common/shaders/vkrbe_vert.glsl")
    set(shader_fragment_glsl "${PROJECT_SOURCE_DIR}/gui/common/shaders/vkrbe_frag.glsl")
    set(shader_vertex_spv "vkrbe_vert.spv.inc")
    set(shader_fragment_spv "vkrbe_frag.spv.inc")
    add_custom_command(COMMENT "Compiling vertex shader"
                       OUTPUT ${shader_vertex_spv}
                       COMMAND ${GLSLANG_VALIDATOR} -V -x -S vert -o ${shader_vertex_spv} ${shader_vertex_glsl}
                       MAIN_DEPENDENCY ${shader_vertex_glsl}
                       DEPENDS ${shader_vertex_glsl} ${GLSLANG_VALIDATOR})
    add_custom_command(COMMENT "Compiling fragment shader"
                       OUTPUT ${shader_fragment_spv}
                       COMMAND ${GLSLANG_VALIDATOR} -V -x -S frag -o ${shader_fragment_spv} ${shader_fragment_glsl}
                       MAIN_DEPENDENCY ${shader_fragment_glsl}
                       DEPENDS ${shader_fragment_glsl} ${GLSLANG_VALIDATOR})
    add_custom_target(compile_shaders DEPENDS ${shader_vertex_spv} ${shader_fragment_spv})
else()
    message(">>> Using GLES renderer")
endif()

if(FRONTEND_TYPE STREQUAL "QT")
    message(">>> Building with Qt5 frontend")
    add_subdirectory("gui/qt5")
elseif(FRONTEND_TYPE STREQUAL "SDL")
    message(">>> Building with SDL frontend")
    add_subdirectory("gui/sdl2")
elseif(NOT FRONTEND_TYPE STREQUAL "NONE")
    message(SEND_ERROR "Unknown frontend specified: ${FRONTEND_TYPE}")
endif()

file(COPY "test/raw.data" DESTINATION "${CMAKE_BINARY_DIR}/bin")
