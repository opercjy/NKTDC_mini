# --- DAQ 프로그램 빌드 ---
add_executable(frontend_tdc_mini frontend_tdc_mini.cpp)

# 라이브러리 링크
target_link_libraries(frontend_tdc_mini
    PRIVATE
    TDC_CONTROLLER
    ${ROOT_LIBRARIES}
)

# --- 캘리브레이션 프로그램 빌드 ---
add_executable(tdc_calibrator tdc_calibrator.cpp)
target_link_libraries(tdc_calibrator PRIVATE TDC_CONTROLLER)

# 생성된 실행 파일 설치
install(TARGETS frontend_tdc_mini tdc_calibrator
    RUNTIME DESTINATION bin
)
