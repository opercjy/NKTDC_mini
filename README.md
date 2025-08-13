-----

# Notice TDC 4채널 DAQ 및 유지보수 프레임워크 (C++ 리팩토링 버전)

## 1\. 개요

이 프로젝트는 **Notice社의 4채널 TDC 모듈**을 위한 데이터 획득(DAQ) 및 유지보수용 소프트웨어 프레임워크입니다. 기존의 C 기반 코드들을 **현대적인 C++17 스타일로 전면 리팩토링**하여 안정성, 유지보수성, 확장성을 크게 향상시켰습니다.

주요 목표는 FADC DAQ 프레임워크와 유사한 사용자 경험을 제공하는 것으로, 데이터는 분석에 용이한 **ROOT TTree 형식**으로 저장되며, 모든 프로그램은 **CMake**를 통해 체계적으로 빌드 및 관리됩니다.

### 주요 구성 요소

  * **`frontend_tdc_mini`**: TDC 데이터를 수집하여 ROOT 파일로 저장하는 메인 DAQ 프로그램.
  * **`tdc_calibrator`**: TDC의 시간 측정 정확도를 보정하고 룩업 테이블(`*.lut`)을 생성하는 유틸리티.
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
│   ├── TdcController.cpp
│   └── TdcController.h
│
├── app/                   # 실행 프로그램 소스
│   ├── frontend_tdc_mini.cpp
│   └── tdc_calibrator.cpp
│
└── scripts/               # 데이터 분석용 ROOT 매크로
    ├── show_distribution.C
    └── show_timing.C
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

빌드가 성공하면 `build/bin` 디렉토리 내에 `frontend_tdc_mini`와 `tdc_calibrator` 실행 파일이 생성됩니다.

### 3.3. 환경 변수 설정 (선택 사항)

`build/bin` 디렉토리를 시스템의 `PATH`에 추가하면 어느 위치에서든 프로그램을 쉽게 실행할 수 있습니다. `~/.bashrc` 파일에 다음 라인을 추가하세요.

```bash
export PATH="/path/to/your/TDC_Project/build/bin:$PATH"
```

-----

## 4\. 사용법

### 4.1. Threshold 설정 (`config/setup.txt`)

DAQ를 시작하기 전, `config/setup.txt` 파일을 수정하여 4개 채널의 판별기(discriminator) 임계값(Threshold)을 설정합니다. [cite\_start]파일은 각 줄에 CH1부터 CH4까지의 임계값을 숫자로 포함해야 합니다. [cite: 2128]

```
10   # CH1 Threshold (1 ~ 255)
10   # CH2 Threshold
10   # CH3 Threshold
10   # CH4 Threshold
```

### 4.2. 데이터 획득 (`frontend_tdc_mini`)

`frontend_tdc_mini`는 설정 파일을 읽어 TDC를 구성한 후, 지정된 시간 동안 데이터를 수집하여 ROOT 파일로 저장합니다.

```bash
# 기본 사용법
frontend_tdc_mini -ip <IP주소> -o <출력파일.root> -c <설정파일> [-t <시간(초)>]

# 예시: 192.168.0.2 TDC로부터 60초간 데이터를 받아 run01.root 파일로 저장
frontend_tdc_mini -ip 192.168.0.2 -o run01.root -c config/setup.txt -t 60
```

#### 저장되는 TTree 구조

  * **Tree 이름**: `tdc_tree`
  * **Branches**:
      * `event_id` (UInt\_t): 이벤트 번호
      * `channel` (UInt\_t): 신호가 들어온 채널 (1\~4)
      * `tdc` (UInt\_t): 원시 TDC 값 (0\~4095)
      * `timestamp` (ULong64\_t): 이벤트의 절대 시간 (단위: 8 ns)

### 4.3. TDC 캘리브레이션 (`tdc_calibrator`)

TDC의 비선형성을 보정하기 위한 룩업 테이블(`.lut`)을 생성합니다.

**실행 전 준비사항**: 캘리브레이션을 진행할 각 채널에 **무작위(random) 신호**를 인가할 수 있도록 준비해야 합니다.

```bash
# 기본 사용법
tdc_calibrator <IP주소> <출력파일.lut>

# 예시
tdc_calibrator 192.168.0.2 tdc_cal.lut
```

프로그램을 실행하면 CH1부터 CH4까지 순서대로 캘리브레이션을 진행하며, 각 채널을 시작하기 전에 사용자에게 신호 연결을 확인하도록 안내합니다. 모든 과정이 끝나면 4개 채널의 보정값이 모두 포함된 `tdc_cal.lut` 파일이 생성됩니다.

### 4.4. 데이터 분석 (`scripts/`)

수집된 데이터나 캘리브레이션 결과를 확인하기 위한 ROOT 매크로 스크립트입니다. 이 스크립트들은 기존 C 프로그램의 분석 방식을 보여주며, 리팩토링된 프로젝트의 ROOT 파일을 분석하기 위해서는 일부 수정이 필요할 수 있습니다.

#### `show_timing.C`

원본 `run_TDC4CH.c`가 생성한 `tdc_4ch.dat` 파일을 읽어 CH1과 CH2 사이의 시간 차이 분포를 보여줍니다. `frontend_tdc_mini`로 생성된 ROOT 파일을 분석하려면, `TFile`과 `TTree`를 열어 데이터를 읽도록 스크립트를 수정해야 합니다.

#### `show_distribution.C`

원본 `calib_TDC4CH.c`가 생성한 `check_calib.txt` 파일을 읽어 캘리브레이션 과정에서 수집된 TDC 값의 원시 분포를 히스토그램으로 보여줍니다. 이를 통해 캘리브레이션이 정상적으로 수행되었는지 시각적으로 확인할 수 있습니다.
