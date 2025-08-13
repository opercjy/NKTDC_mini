// 이전 답변의 tdc_calibrator.cpp 코드와 동일합니다.
// (TdcController.h를 사용하도록 수정된 버전)
#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include "TdcController.h"

bool calibrate_channel(TdcController& tdc, int channel, std::vector<short>& lut) {
    const int TOTAL_EVENTS = 100000;
    std::cout << "\n--- Calibrating Channel " << channel << " ---" << std::endl;
    
    for(int ch=1; ch<=4; ++ch) {
        tdc.setThreshold(ch, (ch == channel) ? 10 : 255);
    }

    tdc.setRawMode(true);
    tdc.reset();
    tdc.setAcquisitionTime(0);
    tdc.start();

    std::vector<int> hist(4096, 0);
    int events_taken = 0;

    std::cout << "Acquiring " << TOTAL_EVENTS << " events..." << std::endl;
    while (events_taken < TOTAL_EVENTS) {
        int data_size = tdc.getDataSize();
        if (data_size > 0) {
            int to_read = std::min(data_size, (TOTAL_EVENTS - events_taken));
            auto data_buffer = tdc.readData(to_read);
            for (int i = 0; i < to_read; ++i) {
                int tdc_val = (static_cast<uint8_t>(data_buffer[i*8+1]) << 8) | static_cast<uint8_t>(data_buffer[i*8]);
                if (tdc_val >= 0 && tdc_val < 4096) {
                    hist[tdc_val]++;
                }
            }
            events_taken += to_read;
            printf("Progress: %d / %d\r", events_taken, TOTAL_EVENTS);
            fflush(stdout);
        }
        usleep(10000);
    }
    
    tdc.stop();
    tdc.setRawMode(false);
    std::cout << "\nData acquisition finished." << std::endl;

    double cnt_all = events_taken;
    double bin_begin = 0.0, bin_end = 0.0;
    double cnt_begin = 0.0, cnt_end = 0.0;

    for (int i = 0; i < 4096; ++i) {
        cnt_end += hist[4095 - i];
        bin_end += ((cnt_end - cnt_begin) / cnt_all * 1000.0);
        lut[4095 - i] = static_cast<short>((bin_end + bin_begin) / 2.0 + 0.5);
        cnt_begin = cnt_end;
        bin_begin = bin_end;
    }
    lut[0] = 0;
    for (int i = 1; i < 4096; ++i) lut[i] += 1;
    
    std::cout << "LUT for channel " << channel << " calculated." << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <TDC_IP_Address> <output.lut>" << std::endl;
        return 1;
    }
    
    std::string ip_addr = argv[1];
    std::string out_filename = argv[2];

    try {
        TdcController tdc;
        tdc.connect(ip_addr);
        tdc.initializeTdc();

        std::ofstream outfile(out_filename, std::ios::binary);
        if (!outfile.is_open()) {
            throw std::runtime_error("Cannot open output file " + out_filename);
        }

        for (int ch = 1; ch <= 4; ++ch) {
            std::vector<short> channel_lut(4096);
            if (!calibrate_channel(tdc, ch, channel_lut)) {
                throw std::runtime_error("Calibration failed for channel " + std::to_string(ch));
            }
            outfile.write(reinterpret_cast<const char*>(channel_lut.data()), 8192);
        }
        std::cout << "\nCalibration complete. Merged LUT saved to " << out_filename << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
