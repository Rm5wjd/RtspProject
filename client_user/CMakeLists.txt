cmake_minimum_required(VERSION 3.10)
project(ClientUser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 자동 처리 활성화
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# =======================
# 1. Qt 찾기 (Qt6 우선, 없으면 Qt5 fallback)
# =======================
find_package(Qt6 COMPONENTS Core Widgets Multimedia OpenGL Quick QuickWidgets REQUIRED QUIET)
if(Qt6_FOUND)
    message(STATUS "Qt6 found: ${Qt6_VERSION}")
    set(QT_TARGET Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::OpenGL Qt6::Quick Qt6::QuickWidgets)
    
    # Quick3D는 선택적 (있으면 사용, 없으면 2D로 대체)
    # Quick3D Tools 의존성 문제를 피하기 위해 직접 찾지 않음
    # 사용자가 설치하면 자동으로 감지됨
    message(STATUS "Note: Qt Quick3D is optional. If installed, 3D rendering will be enabled.")
    message(STATUS "To install: brew install qt6-qtquick3d (or qt5-qtquick3d for Qt5)")
else()
    find_package(Qt5 COMPONENTS Core Widgets Multimedia OpenGL Quick QuickWidgets REQUIRED)
    if(Qt5_FOUND)
        message(STATUS "Qt5 found: ${Qt5_VERSION}")
        set(QT_TARGET Qt5::Core Qt5::Widgets Qt5::Multimedia Qt5::OpenGL Qt5::Quick Qt5::QuickWidgets)
        message(STATUS "Note: Qt Quick3D is optional. If installed, 3D rendering will be enabled.")
    else()
        message(FATAL_ERROR "Qt5 or Qt6 with QuickWidgets is required. Please install Qt with QuickWidgets module.")
    endif()
        endif()

# =======================
# 2. OpenCV 찾기 (macOS arm64 Homebrew 우선)
# =======================
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64| aarch64")
    # Homebrew OpenCV 경로 강제 우선
    set(OpenCV_DIR "/opt/homebrew/opt/opencv/lib/cmake/opencv4" CACHE PATH "OpenCV CMake config path")
    
    # 경로가 실제로 존재하는지 확인
    if(NOT EXISTS "${OpenCV_DIR}/OpenCVConfig.cmake")
        message(FATAL_ERROR "OpenCV CMake config not found at ${OpenCV_DIR}\n"
                            "Please run: brew install opencv\n"
                            "or check brew --prefix opencv")
    endif()
    
    find_package(OpenCV REQUIRED NO_DEFAULT_PATH PATHS "${OpenCV_DIR}")
else()
    find_package(OpenCV REQUIRED)
endif()

if(OpenCV_FOUND)
    message(STATUS "OpenCV version: ${OpenCV_VERSION}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "OpenCV not found.")
endif()

# =======================
# 3. dlib 설정 (MediaPipe 대신 사용)
# =======================
# MediaPipe 설정 (주석 처리)
# set(MEDIAPIPE_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/mediapipe")

# dlib 찾기
find_package(dlib QUIET)
if(dlib_FOUND)
    message(STATUS "dlib found: ${dlib_VERSION}")
    # dlib::dlib 타겟 사용 (CMake 권장 방식)
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
    endif()
endif()

# =======================
# Python 임베딩 설정
# =======================
# 시스템 Python 사용 (venv가 아닌)
set(Python3_FIND_VIRTUALENV FIRST)

find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
if(Python3_FOUND)
    message(STATUS "Python3 found: ${Python3_VERSION}")
    message(STATUS "Python3 executable: ${Python3_EXECUTABLE}")
    message(STATUS "Python3 include dirs: ${Python3_INCLUDE_DIRS}")
    message(STATUS "Python3 libraries: ${Python3_LIBRARIES}")
    
    # NumPy 헤더 경로 확인 및 설정
    if(Python3_NumPy_FOUND)
        message(STATUS "NumPy found: ${Python3_NumPy_VERSION}")
        message(STATUS "NumPy include dirs: ${Python3_NumPy_INCLUDE_DIRS}")
    else()
        # NumPy 헤더 경로를 직접 찾기
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
            OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NUMPY_INCLUDE_DIR)
            set(Python3_NumPy_INCLUDE_DIRS ${NUMPY_INCLUDE_DIR})
            message(STATUS "NumPy include dirs (found manually): ${Python3_NumPy_INCLUDE_DIRS}")
        else()
            message(FATAL_ERROR "NumPy not found. Please install: pip3 install numpy")
        endif()
    endif()
else()
    message(FATAL_ERROR "Python3 with NumPy is required. Please install: pip3 install numpy")
endif()

# 헤더 경로 추가
include_directories(
    ${CMAKE_SOURCE_DIR}/thirdparty/tinygltf
    ${OpenCV_INCLUDE_DIRS}
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
)

# MediaPipe 설정 (주석 처리 - dlib 사용)
# set(MEDIAPIPE_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/mediapipe")
# set(MEDIAPIPE_BUILD_PATHS ...)
# MediaPipe 관련 모든 설정 주석 처리됨

# TinyGLTF 사용 가능 여부 확인
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/tinygltf/tiny_gltf.h")
    add_definitions(-DTINYGLTF_AVAILABLE)
    message(STATUS "TinyGLTF found - GLB file loading enabled")
else()
    message(STATUS "TinyGLTF not found - GLB loading disabled. Download tiny_gltf.h to thirdparty/tinygltf/")
endif()

# MediaPipe 라이브러리 링크 (주석 처리 - dlib 사용)
# set(MEDIAPIPE_LIBS
#     "${MEDIAPIPE_ROOT}/lib/libface_landmarker.a"
# )

# dlib 라이브러리 찾기 및 링크
if(DLIB_FOUND)
    # dlib 라이브러리 경로 찾기
    set(DLIB_LIB_PATHS
        "/opt/homebrew/lib"
        "/opt/homebrew/opt/dlib/lib"
        "/usr/local/lib"
        "/usr/lib"
    )
    
    foreach(DLIB_LIB_PATH ${DLIB_LIB_PATHS})
        if(EXISTS "${DLIB_LIB_PATH}/libdlib.dylib" OR EXISTS "${DLIB_LIB_PATH}/libdlib.a")
            set(DLIB_LIBRARIES "${DLIB_LIB_PATH}/libdlib.dylib")
            if(NOT EXISTS "${DLIB_LIBRARIES}")
                set(DLIB_LIBRARIES "${DLIB_LIB_PATH}/libdlib.a")
            endif()
            message(STATUS "Found dlib library at: ${DLIB_LIBRARIES}")
            break()
        endif()
    endforeach()
    
    if(NOT DLIB_LIBRARIES)
        # dlib을 직접 링크 (시스템 경로에서 찾음)
        set(DLIB_LIBRARIES "dlib")
        message(STATUS "Using system dlib library")
    endif()
else()
    message(WARNING "dlib not found - face landmark detection will be disabled")
    set(DLIB_LIBRARIES "")
endif()

# =======================
# 4. QGst (선택적, 현재 사용하지 않음)
# =======================
# QGst는 현재 사용하지 않으므로 비활성화
set(QGST_FOUND FALSE)
# find_package(PkgConfig QUIET)
# if(PkgConfig_FOUND)
#     pkg_check_modules(QGST QUIET gstreamer-1.0 gstreamer-video-1.0)
#     if(QGST_FOUND)
#         message(STATUS "GStreamer found - QGst 지원 가능")
#         include_directories(${QGST_INCLUDE_DIRS})
#     else()
#         message(STATUS "GStreamer/QtGst not found - OpenCV fallback 사용")
#     endif()
# endif()
message(STATUS "QGst disabled - OpenCV fallback 사용")

# =======================
# 5. 소스 파일 수집 & 실행파일 생성
# =======================
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# VideoGLWidget은 현재 사용하지 않음 (Qt Quick 3D 사용)
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/VideoGLWidget.cpp")
list(REMOVE_ITEM HEADERS "${CMAKE_SOURCE_DIR}/src/VideoGLWidget.h")

add_executable(client_user ${SOURCES} ${HEADERS})

# MOC에 컴파일 정의 전달 (QGST_AVAILABLE 등)
if(QGST_FOUND)
    set_target_properties(client_user PROPERTIES
        AUTOMOC_MOC_OPTIONS "-DQGST_AVAILABLE"
    )
endif()

# QML 파일은 런타임에 직접 로드 (리소스로 포함하지 않음)
if(EXISTS "${CMAKE_SOURCE_DIR}/qml/VideoScene.qml")
    message(STATUS "QML file found: VideoScene.qml (will be loaded at runtime)")
else()
    message(WARNING "QML file not found: qml/VideoScene.qml")
endif()

# =======================
# 6. 링크 라이브러리
# =======================
target_link_libraries(client_user PRIVATE
    ${QT_TARGET}
    ${OpenCV_LIBS}
    # ${MEDIAPIPE_LIBS}  # MediaPipe 라이브러리 (주석 처리)
    ${DLIB_LIBRARIES}  # dlib 라이브러리
    ${Python3_LIBRARIES}  # Python3 라이브러리
)

# dlib이 BLAS/LAPACK을 필요로 하는 경우 추가
if(DLIB_FOUND)
    # macOS에서 Accelerate framework 사용
    if(APPLE)
        target_link_libraries(client_user PRIVATE "-framework Accelerate")
    else()
        # Linux에서 BLAS/LAPACK 링크
        find_library(BLAS_LIB blas)
        find_library(LAPACK_LIB lapack)
        if(BLAS_LIB)
            target_link_libraries(client_user PRIVATE ${BLAS_LIB})
        endif()
        if(LAPACK_LIB)
            target_link_libraries(client_user PRIVATE ${LAPACK_LIB})
        endif()
    endif()
endif()

if(QGST_FOUND)
    target_compile_definitions(client_user PRIVATE QGST_AVAILABLE)
    target_link_libraries(client_user PRIVATE ${QGST_LIBRARIES})
endif()

# macOS에서 rpath 설정 (dylib 찾기)
if(APPLE)
    set_target_properties(client_user PROPERTIES
        BUILD_RPATH "/opt/homebrew/lib;/opt/homebrew/opt/dlib/lib"
        INSTALL_RPATH "/opt/homebrew/lib;/opt/homebrew/opt/dlib/lib"
    )
endif()

# 출력 디렉토리 설정
set_target_properties(client_user PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)