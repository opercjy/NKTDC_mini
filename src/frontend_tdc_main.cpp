#include <iostream>
#include <string>
#include <memory>
#include <csignal>
#include "TdcSystem.h"

// Ctrl+C 신호를 처리하기 위한 전역 TdcSystem 포인터
std::unique_ptr<TdcSystem> g_tdc_system = nullptr;

/**
 * @brief SIGINT (Ctrl+C) 신호가 들어왔을 때 호출될 핸들러 함수.
 * 전역 포인터를 이용해 TdcSystem의 stop() 멤버 함수를 호출합니다.
 */
void signal_handler(int signal) {
    if (g_tdc_system) {
        g_tdc_system->stop();
    }
}

/**
 * @brief 프로그램의 올바른 사용법을 터미널에 출력합니다.
 */
void print_usage() {
    std::cerr << "Usage: frontend_tdc -f <config_file> -o <output_file_base>" << std::endl;
}

/**
 * @brief frontend_tdc 프로그램의 메인 진입점
 */
int main(int argc, char* argv[]) {
    std::string config_file;
    std::string outfile_base;

    // 1. 커맨드라인 인자(-f, -o)를 파싱하여 변수에 저장
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f") {
            if (i + 1 < argc) config_file = argv[++i];
        } else if (arg == "-o") {
            if (i + 1 < argc) outfile_base = argv[++i];
        }
    }

    // 필수 인자가 모두 제공되었는지 확인
    if (config_file.empty() || outfile_base.empty()) {
        print_usage();
        return 1;
    }

    // 2. TdcSystem 객체를 스마트 포인터로 생성하고 전역 포인터에 할당
    g_tdc_system = std::make_unique<TdcSystem>();
    // signal 함수를 이용해 SIGINT 신호가 발생하면 signal_handler를 호출하도록 등록
    signal(SIGINT, signal_handler);

    // 3. DAQ 전체 실행 흐름을 순서대로 제어
    if (!g_tdc_system->loadConfig(config_file)) return 1;
    if (!g_tdc_system->initialize()) return 1;

    g_tdc_system->run(outfile_base);
    
    g_tdc_system->shutdown();

    return 0;
}
