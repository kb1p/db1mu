project(gb1mulator)

option(USE_IMGUI "Use third-party ImGui library (with file dialogs addon) to provide UI" ON)

find_package(SDL2 REQUIRED)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("include"
                    "../common/include"
                    "../../engine/include"
                    ${SDL2_INCLUDE_DIRS}
                    "${SDL2_INCLUDE_DIRS}/SDL2"
                    ${GLES_HDR_PATH})

file(GLOB sources "sources/*.cpp")
file(GLOB headers "include/*.h")

if(USE_VULKAN)
    find_package(Vulkan REQUIRED)

    set(renderer_libs ${Vulkan_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIRS})

    list(APPEND sources "../common/sources/vkrbe.cpp")
else()
    set(GLES_HDR_PATH "$ENV{KHRONOS_HEADERS}" CACHE STRING "Additional directory to search for GLES header files")
    set(GLES_LIB_PATH "$ENV{OPENGLES_LIBDIR}" CACHE STRING "Additional directory to search for GLES libraries")

    include_directories(${GLES_HDR_PATH})

    find_library(renderer_libs NAMES "GLESv2" "GLESv3" HINTS ${GLES_LIB_PATH})
endif()

if(USE_IMGUI)
    add_definitions(-DUSE_IMGUI -DIMGUI_IMPL_OPENGL_ES2 -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)
    include_directories("third_party/imgui"
                        "third_party/imgui/backends"
                        "third_party/imgui/addons")
    file(GLOB imgui_src "third_party/imgui/*.cpp")
    list(APPEND sources ${imgui_src})
    list(APPEND sources "third_party/imgui/backends/imgui_impl_sdl.cpp"
                        "third_party/imgui/addons/imguifilesystem/imguifilesystem.cpp")
    if(RENDERER_TYPE STREQUAL "Vulkan")
        list(APPEND sources "third_party/imgui/backends/imgui_impl_vulkan.cpp")
    else()
        list(APPEND sources "third_party/imgui/backends/imgui_impl_opengl3.cpp")
    endif()
    message(">>> Using ImGui")
endif()

message(">>> Frontend sources: ${sources}")

add_executable(b1mulator ${sources})
target_link_libraries(b1mulator b1-eng ${SDL2_LIBRARIES} ${renderer_libs})

if(USE_VULKAN)
    add_dependencies(b1mulator compile_shaders)
endif()

install(TARGETS b1mulator DESTINATION "bin")
