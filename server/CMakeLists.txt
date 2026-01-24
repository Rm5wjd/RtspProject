cmake_minimum_required(VERSION 3.10)
project(SimpleRtspServer)

set(CMAKE_CXX_STANDARD 17)

# 헤더 경로 추가
include_directories(src src/net src/media src/utils)

# 모든 소스 파일 포함
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(rtsp_server ${SOURCES})
target_link_libraries(rtsp_server pthread)