# CMake 최소 요구 버전 설정
cmake_minimum_required(VERSION 3.10)
# 프로젝트 이름 설정. C++(CXX)와 C언어를 모두 사용하도록 명시합니다.
project(TdcDaqProject CXX C)

# C++14 표준을 사용하도록 설정
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 필수 패키지 및 라이브러리 경로 설정 ---

# ROOT 프레임워크를 찾아 관련 설정을 포함시킵니다.
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

# Notice 라이브러리 경로를 $NKHOME 환경 변수로부터 설정합니다.
include_directories($ENV{NKHOME}/include)
link_directories($ENV{NKHOME}/lib)
# 설치 경로를 프로젝트 폴더 아래 install/ 로 지정
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")


# ==============================================================================
# 타겟 1: C++ DAQ 프로그램 (frontend_tdc)
# ==============================================================================
add_executable(frontend_tdc
    src/frontend_tdc_main.cpp
    src/TdcSystem.cpp
)

# frontend_tdc가 ROOT와 NoticeTDC4CH 라이브러리를 사용하도록 링크합니다.
target_link_libraries(frontend_tdc
    PRIVATE
    ${ROOT_LIBRARIES}
    NoticeTDC4CH
)
# 'make install' 실행 시 bin 디렉터리에 설치되도록 설정
install(TARGETS frontend_tdc DESTINATION bin)


# ==============================================================================
# 타겟 2: C 교정(Calibration) 프로그램 (calibrate_tdc)
# ==============================================================================
# calib_TDC4CH.c 파일을 컴파일하여 calibrate_tdc 라는 실행 파일을 생성합니다.
add_executable(calibrate_tdc calib_TDC4CH.c)

# calibrate_tdc가 NoticeTDC4CH와 수학 라이브러리(m)를 사용하도록 링크합니다.
target_link_libraries(calibrate_tdc
    PRIVATE
    NoticeTDC4CH
    m
)
# 'make install' 실행 시 bin 디렉터리에 설치되도록 설정
install(TARGETS calibrate_tdc DESTINATION bin)


# ==============================================================================
# 사용자 정의 명령어: "make calibrate"
# ==============================================================================
# 'make calibrate'를 입력했을 때 실행될 사용자 정의 명령어를 추가합니다.
add_custom_target(calibrate
    # 먼저 calibrate_tdc 실행 파일을 빌드하는 것에 의존합니다.
    DEPENDS calibrate_tdc
    # 빌드가 완료되면 calibrate_tdc 실행 파일을 터미널에서 실행합니다.
    COMMAND $<TARGET_FILE:calibrate_tdc>
    # 실행 시 터미널에 표시될 메시지
    COMMENT "Running automated TDC calibration for all channels..."
)
