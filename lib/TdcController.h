#ifndef TDC_CONTROLLER_H
#define TDC_CONTROLLER_H

#include <string>
#include <vector>
#include <stdexcept>

/**
 * @class TdcController
 * @brief NoticeKorea 4CH TDC 모듈과의 TCP/IP 통신 및 하드웨어 제어를 캡슐화한 클래스.
 *
 * RAII(Resource Acquisition Is Initialization) 패턴을 따라 객체 소멸 시 자동으로 연결을 해제합니다.
 * 복사 및 이동은 금지되어 유일한 제어 객체임을 보장합니다.
 */
class TdcController {
public:
    TdcController();
    ~TdcController();

    // 복사 및 이동 생성자/대입 연산자 삭제
    TdcController(const TdcController&) = delete;
    TdcController& operator=(const TdcController&) = delete;

    /// @brief TDC에 TCP/IP로 연결합니다.
    void connect(const std::string& ip_address, int port = 5000);
    /// @brief TDC와의 연결을 해제합니다.
    void disconnect();
    /// @brief 연결 상태를 확인합니다.
    bool isConnected() const;

    // --- DAQ 제어 ---
    void reset();
    void start();
    void stop();
    bool isRunning();

    /**
     * @brief DAQ 시간(초)을 TDC 하드웨어에 설정합니다.
     * @note TDC 하드웨어 레지스터는 16비트(0~65535초)만 지원하므로,
     * 이보다 큰 값은 오버플로우되어 예상과 다른 시간으로 설정됩니다.
     * @param seconds 설정할 시간(초). 0으로 설정 시 무한 실행 모드.
     */
    void setAcquisitionTime(int seconds);
    
    // --- 설정 ---
    void setThreshold(int channel, int value);
    int getThreshold(int channel);
    void setRawMode(bool enable);
    void initializeTdc();

    // --- 데이터 읽기 ---
    /// @brief TDC 내부 버퍼에 쌓인 이벤트의 개수를 반환합니다.
    int getDataSize();
    /// @brief 지정된 개수만큼의 이벤트 데이터를 읽어 반환합니다. (1 이벤트 = 8 바이트)
    std::vector<char> readData(int event_count);

private:
    int m_socket_handle = -1; // 소켓 파일 디스크립터

    // 저수준 통신 함수
    void transmit(const char* buffer, int length);
    void receive(char* buffer, int length);
    void writeRegister(int address, int data);
    int readRegister(int address);
};

/// @brief TDC 관련 작업 중 발생하는 오류를 위한 예외 클래스
class TdcError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#endif // TDC_CONTROLLER_H
