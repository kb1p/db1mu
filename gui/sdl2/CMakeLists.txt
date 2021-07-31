project(gb1mulator)

find_package(SDL2 REQUIRED)

find_library(GLES_LIBS NAMES "GLESv2" "libGLESv2")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("include"
                    "../common/include"
                    "../../engine/include"
                    ${SDL2_INCLUDE_DIRS})

file(GLOB sources "sources/*.cpp")
file(GLOB headers "include/*.h")

add_executable(b1mulator ${sources})

target_link_libraries(b1mulator b1-eng ${SDL2_LIBRARIES} ${GLES_LIBS})
