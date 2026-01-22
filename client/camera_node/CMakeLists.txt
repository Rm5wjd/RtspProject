cmake_minimum_required(VERSION 3.10)
project(camera_node)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(camera_sender ${SOURCES})
target_link_libraries(camera_sender pthread)