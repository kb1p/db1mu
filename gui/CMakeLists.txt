project(gb1mulator)

find_package(Qt5 REQUIRED Widgets)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("include")

file(GLOB sources "sources/*.cpp")
file(GLOB forms "forms/*.ui")
file(GLOB headers "include/*.h")

qt5_wrap_cpp(mocs ${headers})
qt5_wrap_ui(uis ${forms})

message("MOCs: ${mocs}")
message("UIs: ${uis}")

add_executable(b1mulator ${sources} ${mocs} ${uis})

target_link_libraries(b1mulator b1-eng Qt5::Widgets)

