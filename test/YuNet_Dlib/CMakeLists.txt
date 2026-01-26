cmake_minimum_required(VERSION 3.16)
project(test_dlib_face)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 옵션: Haar Cascade 사용 여부 (기본값: OFF)
option(USE_HAAR_CASCADE "Use OpenCV Haar Cascade for face detection" OFF)

if(USE_HAAR_CASCADE)
    message(STATUS "Haar Cascade enabled")
    add_definitions(-DUSE_HAAR_CASCADE)
else()
    message(STATUS "Haar Cascade disabled (using dlib only)")
endif()

# OpenCV 찾기
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "OpenCV not found.")
endif()

# dlib 찾기
find_package(dlib QUIET)
if(dlib_FOUND)
    message(STATUS "dlib found: ${dlib_VERSION}")
    set(DLIB_LIBRARIES "dlib::dlib")
    set(DLIB_FOUND TRUE)
else()
    # dlib을 pkg-config로 찾기 시도
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(DLIB QUIET dlib-1)
        if(DLIB_FOUND)
            message(STATUS "dlib found via pkg-config")
            include_directories(${DLIB_INCLUDE_DIRS})
            set(DLIB_FOUND TRUE)
        endif()
    endif()
    
    # pkg-config로 찾지 못한 경우 기본 경로 시도
    if(NOT DLIB_FOUND)
        set(DLIB_POSSIBLE_PATHS
            "/opt/homebrew/include"
            "/opt/homebrew/opt/dlib/include"
            "/usr/local/include"
            "/usr/include"
        )
        
        foreach(DLIB_PATH ${DLIB_POSSIBLE_PATHS})
            if(EXISTS "${DLIB_PATH}/dlib")
                include_directories("${DLIB_PATH}")
                message(STATUS "Found dlib headers at: ${DLIB_PATH}")
                set(DLIB_FOUND TRUE)
                break()
            endif()
        endforeach()
    endif()
    
    if(NOT DLIB_FOUND)
        message(WARNING "dlib not found. Please install: brew install dlib")
        set(DLIB_LIBRARIES "dlib")
    else()
        if(NOT DLIB_LIBRARIES)
            set(DLIB_LIBRARIES "dlib")
        endif()
    endif()
endif()

# 실행 파일 생성
add_executable(test_dlib_face test_dlib_face.cpp)

# 링크 라이브러리
target_link_libraries(test_dlib_face PRIVATE
    ${OpenCV_LIBS}
    ${DLIB_LIBRARIES}
)

# dlib이 BLAS/LAPACK을 필요로 하는 경우 추가
if(DLIB_FOUND)
    # macOS에서 Accelerate framework 사용
    if(APPLE)
        target_link_libraries(test_dlib_face PRIVATE "-framework Accelerate")
    else()
        # Linux에서 BLAS/LAPACK 링크
        find_library(BLAS_LIB blas)
        find_library(LAPACK_LIB lapack)
        if(BLAS_LIB)
            target_link_libraries(test_dlib_face PRIVATE ${BLAS_LIB})
        endif()
        if(LAPACK_LIB)
            target_link_libraries(test_dlib_face PRIVATE ${LAPACK_LIB})
        endif()
    endif()
endif()

# 출력 디렉토리 설정
set_target_properties(test_dlib_face PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
