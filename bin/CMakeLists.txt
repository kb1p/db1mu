ADD_DEFINITIONS(-g -gdwarf-2)

add_executable(db1mu-dbg db1mu-dbg.cpp)
target_link_libraries(db1mu-dbg b1-eng -pthread)