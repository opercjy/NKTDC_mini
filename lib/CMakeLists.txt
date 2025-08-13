# C++ 소스 파일 목록
set(LIB_SOURCES
    TdcController.cpp
)

# 헤더 파일 목록
set(LIB_HEADERS
    TdcController.h
)

# 정적 라이브러리(libTDC.a) 생성
add_library(TDC_CONTROLLER STATIC ${LIB_SOURCES})

# C++17 표준 사용
target_compile_features(TDC_CONTROLLER PUBLIC cxx_std_17)

# 헤더 파일 경로 공개
target_include_directories(TDC_CONTROLLER
    PUBLIC 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# 설치 규칙
install(TARGETS TDC_CONTROLLER ARCHIVE DESTINATION lib)
install(FILES ${LIB_HEADERS} DESTINATION include)
