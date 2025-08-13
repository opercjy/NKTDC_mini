#include "TdcController.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

TdcController::TdcController() : m_socket_handle(-1) {}

TdcController::~TdcController() {
    if (isConnected()) {
        disconnect();
    }
}

void TdcController::connect(const std::string& ip_address, int port) {
    if (isConnected()) {
        disconnect();
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_address.c_str(), &serv_addr.sin_addr) <= 0) {
        throw TdcError("Invalid IP address format");
    }

    m_socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket_handle < 0) {
        throw TdcError("Socket creation failed");
    }

    const int disable_nagle = 1;
    setsockopt(m_socket_handle, IPPROTO_TCP, TCP_NODELAY, &disable_nagle, sizeof(disable_nagle));

    if (::connect(m_socket_handle, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        throw TdcError("Connection to TDC failed: " + std::string(strerror(errno)));
    }
    
    // SPI 비활성화 (기존 코드 참조)
    char cmd_buf[] = { 20 };
    char resp_buf[1];
    transmit(cmd_buf, 1);
    receive(resp_buf, 1);
}

void TdcController::disconnect() {
    if (isConnected()) {
        close(m_socket_handle);
        m_socket_handle = -1;
    }
}

bool TdcController::isConnected() const {
    return m_socket_handle != -1;
}

void TdcController::transmit(const char* buffer, int length) {
    if (!isConnected()) throw TdcError("Not connected to TDC");
    if (write(m_socket_handle, buffer, length) < 0) {
        throw TdcError("Transmit failed: " + std::string(strerror(errno)));
    }
}

void TdcController::receive(char* buffer, int length) {
    if (!isConnected()) throw TdcError("Not connected to TDC");
    int bytes_read = read(m_socket_handle, buffer, length);
    if (bytes_read < length) {
        throw TdcError("Receive failed or incomplete");
    }
}

void TdcController::writeRegister(int address, int data) {
    char buffer[3] = {1, static_cast<char>(address & 0xFF), static_cast<char>(data & 0xFF)};
    char response[1];
    transmit(buffer, 3);
    receive(response, 1);
}

int TdcController::readRegister(int address) {
    char buffer[2] = {2, static_cast<char>(address & 0xFF)};
    char response[1];
    transmit(buffer, 2);
    receive(response, 1);
    return static_cast<uint8_t>(response[0]);
}

void TdcController::reset() { writeRegister(0x0, 0); }
void TdcController::start() { writeRegister(0x1, 1); }
void TdcController::stop() { writeRegister(0x1, 0); }
bool TdcController::isRunning() { return readRegister(0x1) == 1; }

void TdcController::setAcquisitionTime(int seconds) {
    writeRegister(0x2, seconds & 0xFF);
    writeRegister(0x3, (seconds >> 8) & 0xFF);
}

void TdcController::setThreshold(int channel, int value) {
    if (channel < 1 || channel > 4) throw std::out_of_range("Channel must be 1-4");
    writeRegister((channel - 1) + 0x04, value);
}

int TdcController::getThreshold(int channel) {
    if (channel < 1 || channel > 4) throw std::out_of_range("Channel must be 1-4");
    return readRegister((channel - 1) + 0x04);
}

void TdcController::setRawMode(bool enable) {
    writeRegister(0xB, enable ? 1 : 0);
}

void TdcController::initializeTdc() {
    char cmd[] = {4};
    char response[3];
    transmit(cmd, 1);
    receive(response, 3);
    printf("TDC Initialized: Aligned=%d, Delay=%d, Bitslip=%d\n", 
           static_cast<uint8_t>(response[0]), 
           static_cast<uint8_t>(response[1]), 
           static_cast<uint8_t>(response[2]));
}

int TdcController::getDataSize() {
    writeRegister(0x8, 0); // Latch data size
    int lsb = readRegister(0x8);
    int msb = readRegister(0x9);
    return (msb << 8) | lsb;
}

std::vector<char> TdcController::readData(int event_count) {
    int bytes_to_read = event_count * 8;
    std::vector<char> buffer(bytes_to_read);
    
    char cmd[3] = {3, static_cast<char>(bytes_to_read & 0xFF), static_cast<char>((bytes_to_read >> 8) & 0xFF)};
    transmit(cmd, 3);
    receive(buffer.data(), bytes_to_read);
    
    return buffer;
}
