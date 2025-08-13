# --- DAQ 프로그램 빌드 ---
add_executable(frontend_tdc_mini frontend_tdc_mini.cpp)
target_link_libraries(frontend_tdc_mini PRIVATE TDC_CONTROLLER ${ROOT_LIBRARIES})

# \--- 캘리브레이션 프로그램 빌드 ---

add\_executable(tdc\_calibrator tdc\_calibrator.cpp)
target\_link\_libraries(tdc\_calibrator PRIVATE TDC\_CONTROLLER)

# \--- 시각화 프로그램 빌드 ---

add\_executable(tdc\_viewer tdc\_viewer.cpp)
target\_link\_libraries(tdc\_viewer PRIVATE ${ROOT\_LIBRARIES})

# 생성된 실행 파일 설치

install(TARGETS frontend\_tdc\_mini tdc\_calibrator tdc\_viewer
RUNTIME DESTINATION bin
)
