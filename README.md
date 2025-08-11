

# Notice TDC4CH DAQ & Calibration Project

## 1. 개요

이 프로젝트는 Notice 사의 4채널 TDC(Time-to-Digital Converter) 모듈을 위한 데이터 획득(DAQ) 및 교정(Calibration) 소프트웨어 패키지입니다. 프로젝트는 두 가지 주요 실행 파일로 구성됩니다.

1.  `calibrate_tdc`: TDC의 비선형성을 보정하기 위한 조회 테이블(LUT)을 생성하는 완전 자동화된 교정 유틸리티입니다.
2.  `frontend_tdc`: 현대적인 DAQ 워크플로우에 따라 TDC로부터 데이터를 획득하고, 실시간으로 파싱하여 ROOT TTree 형식으로 저장하는 메인 DAQ 프로그램입니다.

## 2. 주요 특징

### frontend_tdc (DAQ 프로그램)
* **통합 워크플로우**: 설정, 데이터 획득, 실시간 파싱, 저장이 프로그램 하나로 통합되어 있습니다.
* **설정 파일 기반**: `tdc_settings.cfg` 텍스트 파일을 통해 재컴파일 없이 DAQ 파라미터를 쉽게 변경할 수 있습니다.
* **ROOT TTree 저장**: 분석에 즉시 사용 가능한 ROOT TTree 형식(`.root`)으로 데이터를 저장합니다.
* **메타데이터 저장**: 실행 시점의 모든 설정값을 ROOT 파일에 함께 저장하여 실험의 재현성을 보장합니다.

### calibrate_tdc (교정 유틸리티)
* **완전 자동화**: 사용자 개입 없이, 단일 명령어로 4개 채널 전체를 순차적으로 교정합니다.
* **단순 명령어 실행**: `make calibrate` 명령어를 통해 빌드와 실행을 한 번에 처리합니다.
* **결과물 생성**: 교정 결과로 `tdc_cal.lut` 파일을 생성합니다.

## 3. 프로젝트 구조


```text
TDC_Project/
├── CMakeLists.txt        \# frontend\_tdc (C++) 빌드용
├── Makefile              \# calibrate\_tdc (C) 빌드 및 실행용
├── calib\_TDC4CH.c        \# 교정 프로그램 소스
├── tdc\_settings.cfg      \# DAQ 설정 파일
└── src/
    ├── TdcSystem.h
    ├── TdcSystem.cpp
    └── frontend\_tdc\_main.cpp
```


## 4. 필요 사항 (Prerequisites)

* **Build Tools**: `gcc`, `g++`, `cmake`, `make`
* **ROOT Framework**: 데이터 저장 및 분석을 위해 필요합니다.
* **NoticeTDC4CH Library**: TDC 하드웨어 제어를 위한 전용 라이브러리. `$NKHOME` 환경 변수 설정이 필요합니다.

## 5. 사용법 (Workflow)

### 1단계: TDC 교정 (최초 1회 또는 필요시)
정확한 시간 측정을 위해 데이터 획득 전 반드시 TDC 교정을 수행해야 합니다.

```bash
# 프로젝트 최상위 디렉터리에서 실행
make calibrate
```

이 명령어는 `calibrate_tdc` 프로그램을 빌드하고 실행하여, 최종적으로 `tdc_cal.lut` 교정 파일을 생성합니다.

### 2단계: DAQ 프로그램 빌드

메인 DAQ 프로그램을 빌드합니다.

```bash
# build 디렉터리 생성 및 이동
mkdir -p build && cd build

# CMake 실행 및 컴파일
cmake ..
make
```

### 3단계: 데이터 획득

`tdc_settings.cfg` 파일을 필요에 맞게 수정한 후, `build` 디렉터리에서 DAQ 프로그램을 실행합니다.

```bash
./bin/frontend_tdc -f ../tdc_settings.cfg -o <출력파일_기본이름>
```

**예시:**

```bash
./bin/frontend_tdc -f ../tdc_settings.cfg -o my_tdc_run
```

\-\> `my_tdc_run.root` 파일이 생성됩니다.

## 6\. 설정 파일 (`tdc_settings.cfg`) 상세

  * `ip_address`: TDC 장비의 IP 주소를 지정합니다.
  * `acq_time_sec`: 데이터 획득 시간을 초 단위로 지정합니다. (`0` = 무한대)
  * `threshold1` \~ `threshold4`: 각 채널의 트리거 문턱값을 1\~255 사이의 값으로 설정합니다.

<!-- end list -->
