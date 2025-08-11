# NKHOME 환경 변수가 설정되어 있어야 함
NKHOME ?= /usr/local/notice

# 컴파일러 및 옵션
CC = gcc
LDFLAGS = -O2 -Wall
LIBS = -L$(NKHOME)/lib -lNoticeTDC4CH -lm
INCLUDEDIR = -I$(NKHOME)/include
COPTS = -Wall $(INCLUDEDIR)

# 대상 파일
CALIB_EXE = calib_TDC4CH.exe
CALIB_SRC = calib_TDC4CH.c

#--- 타겟 정의 ---

# 기본 타겟: 교정 프로그램 실행 파일을 빌드
all: $(CALIB_EXE)

# "make calibrate" 명령어를 위한 타겟
calibrate: $(CALIB_EXE)
	@echo "--- Starting TDC Calibration for all channels (CH1 to CH4) ---"
	./$(CALIB_EXE)
	@echo "--- TDC Calibration Finished. 'tdc_cal.lut' is created. ---"

# 실행 파일 빌드 규칙
$(CALIB_EXE): $(CALIB_SRC)
	$(CC) $(COPTS) $^ -o $@ $(LIBS) $(LDFLAGS)

# 정리
clean:
	rm -f $(CALIB_EXE) *.o *.lut
