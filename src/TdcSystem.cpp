#include "TdcSystem.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h> // for usleep
#include <iomanip>

// ROOT 헤더
#include "TFile.h"
#include "TTree.h"
#include "TTimeStamp.h"

/**
 * @brief TDC의 8바이트 원시(raw) 데이터를 TdcEventData 구조체로 파싱(해석)합니다.
 * @param buffer 8바이트 데이터의 시작점을 가리키는 포인터
 * @param event_data 파싱된 데이터를 채울 TdcEventData 객체의 참조
 * @return 파싱 성공 시 true, 실패 시 false
 */
bool ParseTdcEvent(const char* buffer, TdcEventData& event_data) {
    // const char*를 const uint8_t*로 재해석하여 바이트 단위로 안전하게 접근
    const uint8_t* p = reinterpret_cast<const uint8_t*>(buffer);

    // 8바이트 데이터 구조 (Little-endian 기준):
    // [Byte 0, 1] : 12-bit TDC 값 (Fine time)
    // [Byte 2-6]  : 40-bit Time Tag (Coarse time)
    // [Byte 7]    : 4-bit Channel ID

    event_data.tdc_value = p[0] | (p[1] << 8);

    uint64_t time_tag = 0;
    time_tag |= static_cast<uint64_t>(p[2]);
    time_tag |= static_cast<uint64_t>(p[3]) << 8;
    time_tag |= static_cast<uint64_t>(p[4]) << 16;
    time_tag |= static_cast<uint64_t>(p[5]) << 24;
    time_tag |= static_cast<uint64_t>(p[6]) << 32;
    event_data.time_tag = time_tag;

    event_data.channel = p[7];
    
    // 채널 번호가 유효한지(1-4) 간단히 확인
    return (event_data.channel >= 1 && event_data.channel <= 4);
}

// 생성자: 멤버 변수 초기화
TdcSystem::TdcSystem() : m_tcp_handle(-1), m_is_running(false) {}

// 소멸자: 프로그램 종료 시 연결이 남아있으면 안전하게 종료
TdcSystem::~TdcSystem() {
    if (m_tcp_handle >= 0) {
        shutdown();
    }
}

// 설정 파일 파싱
bool TdcSystem::loadConfig(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Error: Cannot open TDC config file " << config_path << std::endl;
        return false;
    }
    std::string line, key, value;
    while (std::getline(config_file, line)) {
        if (line.empty() || line[0] == '#') continue; // 주석이나 빈 줄은 건너뜀
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos == std::string::npos) continue;
        
        // '='를 기준으로 키와 값 분리 및 공백 제거
        key = line.substr(0, delimiter_pos);
        value = line.substr(delimiter_pos + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 키에 해당하는 값을 m_settings 구조체에 저장
        if (key == "ip_address") m_settings.ip_address = value;
        else if (key == "threshold1") m_settings.thresholds[0] = std::stoi(value);
        else if (key == "threshold2") m_settings.thresholds[1] = std::stoi(value);
        else if (key == "threshold3") m_settings.thresholds[2] = std::stoi(value);
        else if (key == "threshold4") m_settings.thresholds[3] = std::stoi(value);
        else if (key == "acq_time_sec") m_settings.acq_time_sec = std::stoi(value);
    }
    return true;
}

// 하드웨어 연결 및 초기화
bool TdcSystem::initialize() {
    std::cout << "Connecting to TDC at " << m_settings.ip_address << "..." << std::endl;
    // C 스타일 문자열을 요구하는 함수에 맞게 .c_str() 사용
    m_tcp_handle = TDC4CHopen(const_cast<char*>(m_settings.ip_address.c_str()));
    if (m_tcp_handle < 0) {
        std::cerr << "Error: Failed to connect to TDC." << std::endl;
        return false;
    }
    std::cout << "Connection successful. Initializing TDC..." << std::endl;
    TDC4CHinit_TDC(m_tcp_handle); // TDC 내부 ADC 정렬 등 초기화 수행

    // 설정 파일에서 읽은 문턱값(threshold)을 하드웨어에 설정
    for (int i = 0; i < 4; ++i) {
        TDC4CHwrite_THR(m_tcp_handle, i + 1, m_settings.thresholds[i]);
    }
    
    printSettingsSummary(); // 사용자 확인을 위해 설정값 출력
    return true;
}

// 메인 데이터 획득 루프
void TdcSystem::run(const std::string& outfile_base) {
    m_is_running = true;

    // --- ROOT 파일 및 TTree 생성 ---
    std::string root_filename = outfile_base + ".root";
    TFile* outfile = TFile::Open(root_filename.c_str(), "RECREATE");
    if (!outfile || outfile->IsZombie()) {
        std::cerr << "Error: Cannot create output file '" << root_filename << "'" << std::endl;
        return;
    }
    
    // 실행 정보를 담는 'run_info' TTree 생성 및 데이터 저장
    TTree* run_info_tree = new TTree("run_info", "Run Information");
    run_info_tree->Branch("ip_address", &m_settings.ip_address);
    run_info_tree->Branch("thresholds", m_settings.thresholds, "thresholds[4]/I");
    run_info_tree->Branch("acq_time_sec", &m_settings.acq_time_sec, "acq_time_sec/I");
    run_info_tree->Fill();

    // 실제 TDC 히트 데이터를 담는 'tdc_tree' TTree 생성
    TTree* event_tree = new TTree("tdc_tree", "TDC Hit Data");
    TdcEventData event_data;
    event_tree->Branch("event", &event_data.event_number, "event/i");
    event_tree->Branch("ch", &event_data.channel, "ch/b");
    event_tree->Branch("tdc", &event_data.tdc_value, "tdc/s");
    event_tree->Branch("time_tag", &event_data.time_tag, "time_tag/l");
    
    // --- DAQ 시작 ---
    TDC4CHwrite_ACQ_TIME(m_tcp_handle, m_settings.acq_time_sec); // 획득 시간 설정
    TDC4CHreset(m_tcp_handle);  // DAQ 로직 리셋
    TDC4CHstart(m_tcp_handle);  // DAQ 시작
    
    std::cout << "DAQ started. Data will be saved to '" << root_filename << "'" << std::endl;
    TTimeStamp start_time;
    
    std::vector<char> raw_buffer(4096); // 4KB 크기의 임시 소프트웨어 버퍼
    uint32_t total_hits = 0;

    // --- 메인 루프 ---
    while (m_is_running) {
        // 하드웨어의 실행(RUN) 상태를 확인하여 루프 종료 조건 체크
        if (m_settings.acq_time_sec > 0 && !TDC4CHread_RUN(m_tcp_handle)) {
            m_is_running = false;
        }

        // 하드웨어 버퍼에 쌓인 데이터 크기를 확인 (단위: 8바이트 이벤트 개수)
        int data_size = TDC4CHread_DATA_SIZE(m_tcp_handle);
        if (data_size > 0) {
            // 필요한 경우 소프트웨어 버퍼 크기를 동적으로 늘림
            if (raw_buffer.size() < data_size * 8) {
                raw_buffer.resize(data_size * 8);
            }
            // 하드웨어로부터 데이터 읽기
            TDC4CHread_DATA(m_tcp_handle, data_size, raw_buffer.data());

            // 읽어온 데이터 덩어리를 8바이트씩 파싱하여 TTree에 저장
            for (int i = 0; i < data_size; ++i) {
                if (ParseTdcEvent(raw_buffer.data() + i * 8, event_data)) {
                    event_data.event_number = total_hits;
                    event_tree->Fill();
                    total_hits++;
                }
            }
            // 현재까지 처리한 히트 개수를 화면에 표시
            std::cout << "Total hits processed: " << total_hits << "\r" << std::flush;
        }
        usleep(10000); // 10ms 대기, 불필요한 CPU 점유 방지
    }

    // --- DAQ 종료 및 정리 ---
    std::cout << std::endl << "DAQ finished." << std::endl;
    
    // 루프가 끝난 후 하드웨어 버퍼에 남아있을 수 있는 데이터를 마지막으로 한번 더 읽음
    int final_data_size = TDC4CHread_DATA_SIZE(m_tcp_handle);
    if (final_data_size > 0) {
       if (raw_buffer.size() < final_data_size * 8) {
           raw_buffer.resize(final_data_size * 8);
       }
       TDC4CHread_DATA(m_tcp_handle, final_data_size, raw_buffer.data());
       for (int i = 0; i < final_data_size; ++i) {
           if (ParseTdcEvent(raw_buffer.data() + i * 8, event_data)) {
               event_data.event_number = total_hits;
               event_tree->Fill();
               total_hits++;
           }
       }
    }

    // 최종 통계 출력
    TTimeStamp end_time;
    double elapsed_time = end_time.GetSec() - start_time.GetSec() + 1e-9 * (end_time.GetNanoSec() - start_time.GetNanoSec());
    double rate = (elapsed_time > 0) ? total_hits / elapsed_time : 0;
    std::cout << "Total hits: " << total_hits << ", Elapsed Time: " << std::fixed << std::setprecision(2) << elapsed_time << " s, Rate: " << rate << " Hz" << std::endl;

    // ROOT 파일에 TTree 저장 및 파일 닫기
    outfile->cd();
    run_info_tree->Write();
    event_tree->Write();
    outfile->Close();
    delete outfile;

    std::cout << "Data successfully saved to " << root_filename << std::endl;
}

// 하드웨어 연결 종료
void TdcSystem::shutdown() {
    std::cout << "Shutting down TDC system..." << std::endl;
    if (m_tcp_handle >= 0) {
        TDC4CHstop(m_tcp_handle);
        TDC4CHclose(m_tcp_handle);
        m_tcp_handle = -1;
    }
}

// DAQ 루프 중지
void TdcSystem::stop() {
    std::cout << "\nStop signal received. Finalizing DAQ..." << std::endl;
    m_is_running = false;
}

// 설정값 요약 출력
void TdcSystem::printSettingsSummary() {
    std::cout << "---- TDC Settings Summary ----" << std::endl;
    std::cout << " * IP Address: " << m_settings.ip_address << std::endl;
    std::cout << " * Acq. Time: " << m_settings.acq_time_sec << " s" << std::endl;
    for (int i = 0; i < 4; ++i) {
        // 하드웨어에서 직접 설정값을 다시 읽어와 확인
        std::cout << " * CH" << i + 1 << " Threshold: " << TDC4CHread_THR(m_tcp_handle, i + 1) << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
}
