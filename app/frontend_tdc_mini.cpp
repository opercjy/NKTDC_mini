// 이전 답변의 frontend_tdc_mini.cpp 코드와 동일합니다.
// (TdcController.h를 사용하도록 수정된 버전)
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <unistd.h>
#include <fstream>
#include "TdcController.h" // C++ 클래스 헤더
#include "TFile.h"
#include "TTree.h"
#include "TTimeStamp.h"

struct TdcEvent {
    UInt_t event_id = 0;
    UInt_t channel = 0;
    UInt_t tdc = 0;
    ULong64_t timestamp = 0;

    bool parse(const char* buffer, UInt_t& global_event_id) {
        event_id = global_event_id;
        tdc = (static_cast<uint8_t>(buffer[1]) << 8) | static_cast<uint8_t>(buffer[0]);
        timestamp = 0;
        for (int i = 0; i < 5; ++i) {
            timestamp |= static_cast<ULong64_t>(static_cast<uint8_t>(buffer[i + 2])) << (i * 8);
        }
        timestamp *= 8;
        channel = static_cast<uint8_t>(buffer[7]);
        return true;
    }
};

volatile sig_atomic_t g_signal_status = 0;
void signal_handler(int signal) { g_signal_status = signal; }

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " -ip <ip> -o <outfile.root> [-t <sec>] [-c <config.txt>]" << std::endl;
}

int main(int argc, char *argv[]) {
    std::string ip_addr, out_filename, config_filename;
    int acq_time = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-ip") ip_addr = argv[++i];
        else if (arg == "-o") out_filename = argv[++i];
        else if (arg == "-t") acq_time = std::stoi(argv[++i]);
        else if (arg == "-c") config_filename = argv[++i];
    }

    if (ip_addr.empty() || out_filename.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    TdcController tdc;
    try {
        tdc.connect(ip_addr);
        std::cout << "Connected to TDC at " << ip_addr << "." << std::endl;
        tdc.initializeTdc();

        if (!config_filename.empty()) {
            std::ifstream config(config_filename);
            int thr;
            for (int ch = 1; ch <= 4; ++ch) {
                if (config >> thr) {
                    tdc.setThreshold(ch, thr);
                    std::cout << "Set CH" << ch << " threshold to " << tdc.getThreshold(ch) << std::endl;
                }
            }
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
                    event.parse(&data_buffer[i * 8], event_counter);
                    tree->Fill();
                    event_counter++;
                }
                total_events_read += data_size;
                std::cout << "Read " << total_events_read << " events...\r" << std::flush;
            }
            usleep(10000);
        }
        
        std::cout << "\nDAQ finished. Total events saved: " << total_events_read << std::endl;
        outfile->Write();
        outfile->Close();

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
