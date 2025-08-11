#ifndef TDCSYSTEM_H
#define TDCSYSTEM_H

#include <string>
#include <vector>
#include <atomic>
#include <cstdint>

// NoticeTDC4CH.h는 C언어 라이브러리이므로, C++ 코드에 포함시키기 위해 extern "C"를 사용합니다.
extern "C" {
#include "NoticeTDC4CH.h"
}

/**
 * @struct TdcSettings
 * @brief TDC 설정 파일(tdc_settings.cfg)에서 읽어올 파라미터를 저장하는 구조체
 */
struct TdcSettings {
    std::string ip_address = "192.168.0.2";
    int thresholds[4] = {10, 10, 10, 10};
    int acq_time_sec = 10; // 데이터 획득 시간 (초)
};

/**
 * @struct TdcEventData
 * @brief ROOT TTree에 저장할 단일 TDC 히트(hit) 데이터 구조체
 */
struct TdcEventData {
    uint32_t event_number; // 전체 히트 중 몇 번째 히트인지 나타내는 번호
    uint8_t  channel;      // 신호가 들어온 채널 (1-4)
    uint16_t tdc_value;    // 12-bit 정밀 시간 측정값 (Fine time)
    uint64_t time_tag;     // 40-bit 전체 시간 측정값 (Coarse time)
};

/**
 * @class TdcSystem
 * @brief TDC DAQ의 연결, 설정, 데이터 획득, 종료 등 모든 기능을 캡슐화하는 클래스
 */
class TdcSystem {
public:
    // 생성자 및 소멸자
    TdcSystem();
    ~TdcSystem();

    /**
     * @brief 설정 파일을 읽어 m_settings 멤버 변수에 저장합니다.
     * @param config_path 설정 파일의 경로
     * @return 성공 시 true, 실패 시 false
     */
    bool loadConfig(const std::string& config_path);

    /**
     * @brief TDC 하드웨어에 TCP/IP로 연결하고, 설정 파일의 값들을 적용합니다.
     * @return 성공 시 true, 실패 시 false
     */
    bool initialize();

    /**
     * @brief 메인 데이터 획득 루프를 실행합니다.
     * @param outfile_base 출력될 ROOT 파일의 기본 이름
     */
    void run(const std::string& outfile_base);

    /**
     * @brief DAQ를 중지하고 하드웨어 연결을 안전하게 종료합니다.
     */
    void shutdown();

    /**
     * @brief 외부 신호(Ctrl+C)를 통해 DAQ 루프를 안전하게 중지시키기 위한 함수
     */
    void stop();

private:
    TdcSettings m_settings;      // DAQ 설정을 저장하는 멤버 변수
    int m_tcp_handle;            // TDC와의 TCP/IP 연결 핸들
    std::atomic<bool> m_is_running; // DAQ 루프의 실행 상태를 제어하는 플래그

    /**
     * @brief 현재 적용된 설정값들을 터미널에 출력하는 헬퍼 함수
     */
    void printSettingsSummary();
};

#endif // TDCSYSTEM_H
