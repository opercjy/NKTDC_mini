#ifndef TDC_CONTROLLER_H
#define TDC_CONTROLLER_H

#include <string>
#include <vector>
#include <stdexcept>

// TDC 통신 및 제어를 위한 C++ 클래스
class TdcController {
public:
    // 소멸자에서 자동으로 연결을 해제 (RAII)
    TdcController();
    ~TdcController();

    // 복사 및 이동 금지
    TdcController(const TdcController&) = delete;
    TdcController& operator=(const TdcController&) = delete;

    // TDC에 연결
    void connect(const std::string& ip_address, int port = 5000);
    // TDC 연결 해제
    void disconnect();
    bool isConnected() const;

    // DAQ 제어
    void reset();
    void start();
    void stop();
    bool isRunning();
    void setAcquisitionTime(int seconds);
    
    // 설정
    void setThreshold(int channel, int value);
    int getThreshold(int channel);
    void setRawMode(bool enable);
    void initializeTdc();

    // 데이터 읽기
    int getDataSize();
    std::vector<char> readData(int event_count);

private:
    int m_socket_handle = -1;

    // 저수준 통신 함수
    void transmit(const char* buffer, int length);
    void receive(char* buffer, int length);
    void writeRegister(int address, int data);
    int readRegister(int address);
};

// 예외 클래스
class TdcError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#endif // TDC_CONTROLLER_H
