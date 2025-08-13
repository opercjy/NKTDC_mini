cmake_minimum_required(VERSION 3.10)
project(TDC_Project CXX C)

# C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ROOT 패키지 찾기
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

# 모든 실행 파일을 build/bin 디렉토리에 생성
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 하위 디렉토리 추가
add_subdirectory(lib)
add_subdirectory(app)

message(STATUS "All executables will be created in: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
