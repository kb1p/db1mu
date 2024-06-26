project(gb1mulator)

find_package(Qt5 REQUIRED Widgets Multimedia)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("include"
                    "../common/include"
                    "../../engine/include")

file(GLOB sources "sources/*.cpp")
file(GLOB forms "forms/*.ui")
file(GLOB headers "include/*.h")

if(USE_VULKAN)
    find_package(Vulkan REQUIRED)

    set(renderer_libs ${Vulkan_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIRS})

    list(APPEND sources "../common/sources/vkrbe.cpp")
else()
    set(renderer_libs)
endif()

qt5_wrap_cpp(mocs ${headers})
qt5_wrap_ui(uis ${forms})

message("MOCs: ${mocs}")
message("UIs: ${uis}")

add_executable(b1mulator ${sources} ${mocs} ${uis})

if(USE_VULKAN)
    add_dependencies(b1mulator compile_shaders)
endif()

target_link_libraries(b1mulator b1-eng ${renderer_libs} Qt5::Widgets Qt5::Multimedia)

install(TARGETS b1mulator DESTINATION "bin")
