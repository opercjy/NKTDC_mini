# --- DAQ 프로그램 빌드 ---
add_executable(frontend_tdc_mini frontend_tdc_mini.cpp)
target_link_libraries(frontend_tdc_mini PRIVATE TDC_CONTROLLER ${ROOT_LIBRARIES})

# --- 캘리브레이션 프로그램 빌드 ---

add_executable(tdc_calibrator tdc_calibrator.cpp)
target_link_libraries(tdc_calibrator PRIVATE TDC_CONTROLLER)

# --- 시각화 프로그램 빌드 ---

add_executable(tdc_viewer tdc_viewer.cpp)
target_link_libraries(tdc_viewer PRIVATE ${ROOT_LIBRARIES})

# --- 3채널 기반 뮤온 수명 분석 프로그램 빌드 ---

add_executable(measure_lifetime measure_lifetime.cpp)
target_link_libraries(measure_lifetime PRIVATE ${ROOT_LIBRARIES})

# 생성된 실행 파일 설치

install(TARGETS frontend_tdc_mini tdc_calibrator tdc_viewer measure_lifetime RUNTIME DESTINATION bin)
