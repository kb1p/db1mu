cmake_minimum_required(VERSION 3.1)

project(b1mulator)

option(BUILD_DEBUGGER "Build command line-based debugger" OFF)
set(FRONTEND_TYPE "SDL" CACHE STRING "Which frontend to use [SDL, QT, NONE]")
set(RENDERER_TYPE "GLES" CACHE STRING "Which renderer to use [GLES, Vulkan]")

include_directories("engine/include")

set(CMAKE_CXX_STANDARD 11)
#add_compile_options(-Wall -Werror -Wl,--no-undefined)

add_subdirectory("engine")

if(BUILD_DEBUGGER)
    add_subdirectory("bin")
endif()

if(RENDERER_TYPE STREQUAL "Vulkan")
    message(">>> Using Vulkan renderer")
    add_definitions(-DUSE_VULKAN)
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
