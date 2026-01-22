cmake_minimum_required(VERSION 3.10)
project(ClientUser)

set(CMAKE_CXX_STANDARD 17)

# OpenCV - Mac M1 (Apple Silicon) 지원
# /usr/local의 오래된 OpenCV 설정을 무시하고 올바른 경로를 찾도록 함
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    # Mac M1: Homebrew는 /opt/homebrew에 설치됨
    # 여러 가능한 경로를 시도
    set(OpenCV_POSSIBLE_PATHS
        "/opt/homebrew/opt/opencv/lib/cmake/opencv4"
        "/opt/homebrew/lib/cmake/opencv4"
        "/opt/homebrew/share/opencv4"
    )
    
    # 먼저 가능한 경로에서 찾기
    set(OpenCV_FOUND_PATH "")
    foreach(PATH ${OpenCV_POSSIBLE_PATHS})
        if(EXISTS "${PATH}/OpenCVConfig.cmake")
            set(OpenCV_DIR "${PATH}" CACHE PATH "OpenCV config directory" FORCE)
            set(OpenCV_FOUND_PATH "${PATH}")
            message(STATUS "Found OpenCV at: ${OpenCV_DIR}")
            break()
        endif()
    endforeach()
    
    # 경로를 찾지 못했다면 Homebrew prefix에서 검색
    if(NOT OpenCV_FOUND_PATH)
        execute_process(
            COMMAND brew --prefix opencv
            OUTPUT_VARIABLE HOMEBREW_OPENCV_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(HOMEBREW_OPENCV_PREFIX AND EXISTS "${HOMEBREW_OPENCV_PREFIX}/lib/cmake/opencv4/OpenCVConfig.cmake")
            set(OpenCV_DIR "${HOMEBREW_OPENCV_PREFIX}/lib/cmake/opencv4" CACHE PATH "OpenCV config directory" FORCE)
            message(STATUS "Found OpenCV at: ${OpenCV_DIR}")
        endif()
    endif()
    
    # NO_DEFAULT_PATH를 사용하여 /usr/local의 오래된 설정을 완전히 무시
    if(OpenCV_DIR)
        find_package(OpenCV NO_DEFAULT_PATH PATHS ${OpenCV_DIR} REQUIRED)
    else()
        message(FATAL_ERROR "OpenCV not found. Please install OpenCV using: brew install opencv\n"
                            "If OpenCV is installed, you may need to remove old configuration files:\n"
                            "  sudo rm -rf /usr/local/lib/cmake/opencv4")
    endif()
else()
    find_package(OpenCV REQUIRED)
endif()

if(OpenCV_FOUND)
    message(STATUS "OpenCV version: ${OpenCV_VERSION}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV using: brew install opencv")
endif()
include_directories(${OpenCV_INCLUDE_DIRS})

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW - Mac M1 지원
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(glfw3_DIR "/opt/homebrew/lib/cmake/glfw3" CACHE PATH "GLFW3 config directory")
endif()
find_package(glfw3 REQUIRED)

# GLEW - Mac M1 지원
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(GLEW_ROOT_DIR "/opt/homebrew/opt/glew" CACHE PATH "GLEW root directory")
endif()
find_package(GLEW REQUIRED)

# Include directories
include_directories(src)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(client_user ${SOURCES})

# Link libraries
target_link_libraries(client_user 
    ${OpenCV_LIBS}
    OpenGL::GL
    glfw
    GLEW::GLEW
)

# Set output directory
set_target_properties(client_user PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
