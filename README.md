-----

# Notice TDC 4채널 DAQ 및 분석 프레임워크 (C++ 리팩토링 버전)

## 1\. 개요

이 프로젝트는 **NoticeKorea의 4채널 TDC 모듈**을 위한 데이터 획득(DAQ) 및 유지보수용 소프트웨어 프레임워크입니다. 기존의 C 기반 코드들을 **현대적인 C++17 스타일로 전면 리팩토링**하여 안정성, 유지보수성, 확장성을 크게 향상시켰습니다.

데이터는 분석에 용이한 **ROOT TTree 형식**으로 저장되며, 모든 프로그램은 **CMake**를 통해 체계적으로 빌드 및 관리됩니다.

### 1.1. 트리거 방식: Constant Fraction Discrimination (CFD)

본 TDC 모듈은 제조사 정보에 따라, 신호의 크기에 상관없이 일정한 타이밍 정보를 얻는 데 유리한 **CFD(Constant Fraction Discrimination)** 방식을 사용하여 이벤트를 트리거합니다. 이는 단순 전압 레벨을 비교하는 FADC의 LED(Leading Edge Discrimination) 방식과 구분되는 중요한 특징입니다.

### 1.2. 주요 구성 요소

  * **`frontend_tdc_mini`**: TDC 데이터를 수집하여 ROOT 파일로 저장하는 메인 DAQ 프로그램.
  * **`tdc_calibrator`**: TDC의 시간 측정 정확도를 보정하고 룩업 테이블(`*.lut`)을 생성하는 유틸리티.
  * **`tdc_viewer`**: 저장된 TTree 데이터를 시각화하고 기본 분석을 수행하는 프로그램.
  * **`libTDC_CONTROLLER.a`**: TDC와의 TCP/IP 통신을 캡슐화한 핵심 C++ 정적 라이브러리.

-----

## 2\. 프로젝트 구조

```

TDC_Project/
├── CMakeLists.txt         # 메인 빌드 스크립트
├── README.md              # 본 문서
│
├── config/
│   └── setup.txt          # 채널별 Threshold 설정 파일
│
├── lib/                   # 핵심 라이브러리 소스
│   └── TdcController.cpp/h
│
└── app/                   # 실행 프로그램 소스
├── frontend_tdc_mini.cpp
├── tdc_calibrator.cpp
└── tdc_viewer.cpp

```

-----

## 3\. 빌드 및 설치

### 3.1. 시스템 요구사항

  * **운영체제**: Linux
  * **빌드 도구**: `cmake` (v3.10 이상), `g++` (C++17 지원)
  * **필수 라이브러리**: `ROOT 6`

### 3.2. 빌드 절차

프로젝트 최상위 디렉토리에서 아래 명령어를 실행하여 모든 프로그램을 빌드합니다.

```bash
# 1. 빌드 디렉토리 생성 및 이동
mkdir -p build && cd build

# 2. CMake 실행하여 빌드 환경 구성
cmake ..

# 3. 컴파일
make
```

빌드가 성공하면 `build/bin` 디렉토리 내에 `frontend_tdc_mini`, `tdc_calibrator`, `tdc_viewer` 실행 파일이 생성됩니다.

-----

## 4\. 사용법

### 4.1. DAQ 설정 (`config/setup.txt`)

DAQ를 시작하기 전, `config/setup.txt` 파일을 수정하여 **TDC의 IP 주소**와 \*\*채널별 임계값(Threshold)\*\*을 설정합니다. (CFD 방식에서도 arming을 위한 임계값은 필요합니다.)

```text
# TDC Device IP Address
192.168.0.2

# Thresholds (1 ~ 255)
10   # CH1 
10   # CH2
10   # CH3
10   # CH4
```

### 4.2. 데이터 획득 (`frontend_tdc_mini`)

설정 파일을 읽어 DAQ를 수행하고, 데이터를 ROOT TTree 형식으로 저장합니다.

```bash
# 기본 사용법
frontend_tdc_mini -c <설정파일> -o <출력파일.root> [-t <시간(초)>]

# 예시
frontend_tdc_mini -c config/setup.txt -o run01.root -t 60
```

### 4.3. 데이터 시각화 (`tdc_viewer`)

획득한 `.root` 파일을 열어 기본적인 분석 결과를 확인합니다.

```bash
# 사용법
tdc_viewer <입력파일.root>

# 예시
tdc_viewer run01.root
```

프로그램을 실행하면 채널별 Hit 분포, TDC 스펙트럼, CH1-CH2 시간차 분포 등을 담은 캔버스가 나타납니다.

### 4.4. TDC 캘리브레이션 (`tdc_calibrator`)

TDC의 비선형성을 보정하기 위한 룩업 테이블(`.lut`)을 생성합니다.

**준비사항**: 캘리브레이션할 각 채널에 **무작위(random) 신호**를 인가해야 합니다.

```bash
# 사용법
tdc_calibrator <IP주소> <출력파일.lut>

# 예시
tdc_calibrator 192.168.0.2 tdc_cal.lut
```

프로그램의 안내에 따라 CH1부터 CH4까지 순서대로 신호를 연결하며 캘리브레이션을 진행합니다.
