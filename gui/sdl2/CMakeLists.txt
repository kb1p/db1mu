project(gb1mulator)

option(USE_IMGUI "Use third-party ImGui library (with file dialogs addon) to provide UI" ON)

find_package(SDL2 REQUIRED)

set(GLES_HDR_PATH "$ENV{KHRONOS_HEADERS}" CACHE STRING "Additional directory to search for GLES header files")
set(GLES_LIB_PATH "$ENV{OPENGLES_LIBDIR}" CACHE STRING "Additional directory to search for GLES libraries")

find_library(GLES_LIBS NAMES "GLESv2" "GLESv3" HINTS ${GLES_LIB_PATH})

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("include"
                    "../common/include"
                    "../../engine/include"
                    ${SDL2_INCLUDE_DIRS}
                    "${SDL2_INCLUDE_DIRS}/SDL2"
                    ${GLES_HDR_PATH})

file(GLOB sources "sources/*.cpp")
file(GLOB headers "include/*.h")

if(RENDERER_TYPE STREQUAL "Vulkan")
    list(APPEND sources "../common/vkrbe.cpp")
endif()

if(USE_IMGUI)
    add_definitions(-DUSE_IMGUI -DIMGUI_IMPL_OPENGL_ES2 -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)
    include_directories("third_party/imgui"
                        "third_party/imgui/backends"
                        "third_party/imgui/addons")
    file(GLOB imgui_src "third_party/imgui/*.cpp")
    list(APPEND sources ${imgui_src})
    list(APPEND sources "third_party/imgui/backends/imgui_impl_opengl3.cpp"
                        "third_party/imgui/backends/imgui_impl_sdl.cpp"
                        "third_party/imgui/addons/imguifilesystem/imguifilesystem.cpp")
    message(">>> Using ImGui")
endif()

message(">>> Frontend sources: ${sources}")

add_executable(b1mulator ${sources})

target_link_libraries(b1mulator b1-eng ${SDL2_LIBRARIES} ${GLES_LIBS})

install(TARGETS b1mulator DESTINATION "bin")
