/**
 * @file frontend_tdc_mini.cpp
 * @brief TDC로부터 데이터를 수집하여 ROOT TTree 형식으로 저장하는 메인 DAQ 프로그램.
 *
 * TDC 하드웨어로부터 생성된 hit 데이터를 시간순으로 (list mode) 저장합니다.
 * Ctrl+C (SIGINT) 시그널을 처리하여 데이터 손실 없이 안전하게 종료하는 기능이 포함되어 있습니다.
 * ROOT TTree는 내부적으로 자동 저장(Auto-Save/Flush) 메커니즘을 가지고 있어,
 * 프로그램이 비정상 종료되어도 대부분의 데이터는 안전하게 보존됩니다.
 */
#include "TdcController.h"
#include "TFile.h"
#include "TTree.h"
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <unistd.h>
#include <fstream>
#include <sstream>

/// @brief TDC의 8바이트 raw 데이터를 파싱하여 TTree에 저장하기 위한 구조체
struct TdcEvent {
    UInt_t    event_id = 0;
    UInt_t    channel = 0;
    UInt_t    tdc = 0;
    ULong64_t timestamp = 0;

    /**
     * @brief 8바이트 버퍼를 파싱하여 멤버 변수를 채웁니다.
     * @param buffer TDC raw 데이터 포인터 (8바이트)
     * @param global_event_id 전역 이벤트 카운터
     */
    void parse(const char* buffer, UInt_t& global_event_id) {
        event_id = global_event_id;
        // LSB first
        tdc = (static_cast<uint8_t>(buffer[1]) << 8) | static_cast<uint8_t>(buffer[0]);
        timestamp = 0;
        for (int i = 0; i < 5; ++i) {
            timestamp |= static_cast<ULong64_t>(static_cast<uint8_t>(buffer[i + 2])) << (i * 8);
        }
        // 하드웨어 timestamp 단위가 8ps이므로 TTree에는 ps 단위로 변환하여 저장
        timestamp *= 8;
        channel = static_cast<uint8_t>(buffer[7]);
    }
};

// Ctrl+C 시그널 처리를 위한 전역 변수
volatile sig_atomic_t g_signal_status = 0;
void signal_handler(int signal) { g_signal_status = signal; }

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " -o <outfile.root> -c <config.txt> [-t <sec>] [-ip <ip_override>]" << std::endl;
}

int main(int argc, char *argv[]) {
    std::string ip_addr, out_filename, config_filename;
    int acq_time = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o") out_filename = argv[++i];
        else if (arg == "-c") config_filename = argv[++i];
        else if (arg == "-t") acq_time = std::stoi(argv[++i]);
        else if (arg == "-ip") ip_addr = argv[++i];
    }

    if (out_filename.empty() || config_filename.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    // --- 설정 파일 파싱 ---
    std::ifstream config_file(config_filename);
    if (!config_file.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_filename << std::endl;
        return 1;
    }

    std::string line;
    std::string config_ip;
    std::vector<int> thresholds;
    
    while(std::getline(config_file, line)) {
        if (line.empty() || line[0] == '#') continue;
        config_ip = line;
        break;
    }
    while(std::getline(config_file, line) && thresholds.size() < 4) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        int thr;
        if (ss >> thr) thresholds.push_back(thr);
    }
    
    if (ip_addr.empty()) ip_addr = config_ip;

    if (ip_addr.empty() || thresholds.size() < 4) {
        std::cerr << "Error: Invalid config file format. IP address and 4 thresholds are required." << std::endl;
        return 1;
    }

    // --- DAQ 로직 시작 ---
    TdcController tdc;
    try {
        tdc.connect(ip_addr);
        tdc.initializeTdc();

        for (int ch = 1; ch <= 4; ++ch) {
            tdc.setThreshold(ch, thresholds[ch-1]);
        }

        TFile* outfile = TFile::Open(out_filename.c_str(), "RECREATE");
        TTree* tree = new TTree("tdc_tree", "TDC4CH Data");
        TdcEvent event;
        tree->Branch("event_id", &event.event_id);
        tree->Branch("channel", &event.channel);
        tree->Branch("tdc", &event.tdc);
        tree->Branch("timestamp", &event.timestamp);

        signal(SIGINT, signal_handler);
        tdc.setAcquisitionTime(acq_time);
        tdc.reset();
        tdc.start();
        std::cout << "DAQ started. Press Ctrl+C to stop." << std::endl;

        UInt_t event_counter = 0;
        long total_events_read = 0;
        
        while (tdc.isRunning() && !g_signal_status) {
            int data_size = tdc.getDataSize();
            if (data_size > 0) {
                auto data_buffer = tdc.readData(data_size);
                for (int i = 0; i < data_size; ++i) {
                    // TTree::Fill()은 내부적으로 '바스켓(Basket)'이라는 메모리 버퍼에 데이터를 채웁니다.
                    // 이 바스켓이 가득 차면 ROOT가 자동으로 파일에 데이터를 쓰는 'Auto-Flush' 기능이 동작하므로,
                    // 프로그램이 비정상적으로 종료되어도 대부분의 데이터는 안전하게 보존됩니다.
                    event.parse(&data_buffer[i * 8], event_counter);
                    tree->Fill();
                    event_counter++;
                }
                total_events_read += data_size;
                std::cout << "Read " << total_events_read << " events...\r" << std::flush;
            }
            usleep(10000); // 10ms 대기 (CPU 부하 감소)
        }
        
        // DAQ 루프가 모두 끝난 후, 메모리 버퍼에 남아있는 마지막 데이터를 모두 파일에 기록합니다.
        std::cout << "\nDAQ finished. Total events saved: " << total_events_read << std::endl;
        outfile->Write();
        outfile->Close();

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
