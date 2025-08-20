/**
 * @file measure_lifetime.cpp
 * @brief 원본 TDC 데이터를 분석하여 뮤온(muon)의 수명을 측정하는 스크립트.
 *
 * 이 프로그램은 '상태 머신(State Machine)' 방식으로 동작하며,
 * 시간순으로 정렬된 TDC hit 데이터를 읽어 README에 기술된 로직에 따라 유효한 뮤온 붕괴 이벤트를 찾습니다.
 *
 * --- 측정 로직 ---
 * 1. Start: CH1(A)과 CH2(B)의 동시 신호, CH3(C) 없음. (뮤온이 검출기 통과 후 정지)
 * 2. End: CH2(B)에서만 단일 신호. (정지한 뮤온이 붕괴)
 * 3. Abort: Start 이후 End 이전에 CH1(A) 또는 CH3(C)에서 신호 발생 시 측정 무효화.
 *
 * 사용자는 '-d' 옵션을 통해 Start 신호 직후의 노이즈를 무시하는 'Decay Gate' 시간을 설정할 수 있습니다.
 */
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>

/// @brief 시간적으로 인접한 여러 hit을 하나의 물리적 이벤트로 묶기 위한 구조체
struct Hit {
    UInt_t channel;
    ULong64_t timestamp;
};

void measure_lifetime(const std::string& infile_name, const std::string& outfile_name, int delay_ns) {
    TFile* infile = TFile::Open(infile_name.c_str(), "READ");
    if (!infile || infile->IsZombie()) {
        std::cerr << "Error opening input file: " << infile_name << std::endl;
        return;
    }
    TTreeReader reader("tdc_tree", infile);

    TTreeReaderValue<UInt_t> channel(reader, "channel");
    TTreeReaderValue<ULong64_t> timestamp(reader, "timestamp");

    TFile* outfile = new TFile(outfile_name.c_str(), "RECREATE");
    TTree* outtree = new TTree("lifetime_tree", "Muon Lifetime Data");
    double lifetime_ps; // 계산된 수명을 피코초 단위로 저장
    outtree->Branch("lifetime_ps", &lifetime_ps);

    // --- 상태 머신을 위한 변수 및 상수 ---
    enum class State { WAITING_FOR_START, WAITING_FOR_END };
    State currentState = State::WAITING_FOR_START;
    
    ULong64_t start_timestamp = 0; // Start 이벤트의 타임스탬프 저장
    
    // Coincidence window (하나의 이벤트로 묶는 시간): 100 ns
    const ULong64_t coincidence_window = 100000; // 100,000 ps
    // Max lifetime (이 시간 안에 붕괴 안하면 Abort 처리): 20 us
    const ULong64_t max_lifetime_window = 20000000; // 20,000,000 ps
    // Decay Gate (Start 이후 이 시간 동안은 End 신호 무시): 사용자 설정
    const ULong64_t decay_gate_start_window = static_cast<ULong64_t>(delay_ns) * 1000; // ns to ps

    std::vector<Hit> current_event_hits; // 이벤트 빌딩을 위한 임시 버퍼
    long total_entries = reader.GetEntries();
    long processed_entries = 0;
    int successful_decays = 0;

    // --- 메인 루프: 모든 hit을 순회하며 상태 머신 동작 ---
    while (reader.Next()) {
        processed_entries++;
        if (processed_entries % 100000 == 0) {
            printf("Processing... %ld / %ld\r", processed_entries, total_entries);
            fflush(stdout);
        }

        // --- 1. 이벤트 빌딩: 시간적으로 가까운 hit들을 묶음 ---
        if (!current_event_hits.empty() && (*timestamp - current_event_hits.front().timestamp > coincidence_window)) {
            bool hasA = false, hasB = false, hasC = false;
            for (const auto& hit : current_event_hits) {
                if (hit.channel == 1) hasA = true;
                if (hit.channel == 2) hasB = true;
                if (hit.channel == 3) hasC = true;
            }
            ULong64_t event_time = current_event_hits.front().timestamp;

            // --- 2. 상태 머신 로직 ---
            if (currentState == State::WAITING_FOR_START) {
                // Start Logic: CH1(A) & CH2(B) & !CH3(C)
                if (hasA && hasB && !hasC) {
                    currentState = State::WAITING_FOR_END; // 상태 전환: ARMED
                    start_timestamp = event_time;
                }
            } else if (currentState == State::WAITING_FOR_END) {
                // Decay Gate: 설정된 delay 시간 이내의 신호는 무시
                if (event_time - start_timestamp < decay_gate_start_window) {
                    // 아무것도 하지 않고 다음 이벤트를 기다림 (신호를 무시함)
                }
                // Timeout 또는 Abort Logic 확인
                else if (event_time - start_timestamp > max_lifetime_window || (hasA || hasC)) {
                    currentState = State::WAITING_FOR_START; // 리셋
                }
                // End Logic: !CH1(A) & CH2(B) & !CH3(C)
                else if (hasB && !hasA && !hasC) {
                    lifetime_ps = static_cast<double>(event_time - start_timestamp);
                    outtree->Fill(); // 성공! 수명 기록
                    successful_decays++;
                    currentState = State::WAITING_FOR_START; // 다음 측정을 위해 리셋
                }
            }
            
            current_event_hits.clear();
        }
        current_event_hits.push_back({*channel, *timestamp});
    }
    
    // 마지막 이벤트 처리
    if(!current_event_hits.empty()){
        bool hasA = false, hasB = false, hasC = false;
        for (const auto& hit : current_event_hits) {
            if (hit.channel == 1) hasA = true;
            if (hit.channel == 2) hasB = true;
            if (hit.channel == 3) hasC = true;
        }
        ULong64_t event_time = current_event_hits.front().timestamp;
        if (currentState == State::WAITING_FOR_END) {
             if (event_time - start_timestamp > decay_gate_start_window &&
                 event_time - start_timestamp <= max_lifetime_window &&
                 (hasB && !hasA && !hasC)) {
                    lifetime_ps = static_cast<double>(event_time - start_timestamp);
                    outtree->Fill();
                    successful_decays++;
             }
        }
    }

    outfile->Write();
    outfile->Close();
    infile->Close();
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <input.root> <output.root> [-d <delay_ns>]" << std::endl;
        return 1;
    }
    
    std::string infile = argv[1];
    std::string outfile = argv[2];
    int delay_ns = 0; // 기본값은 0 ns (게이트 없음)

    if (argc == 5) {
        if (std::string(argv[3]) == "-d") {
            try {
                delay_ns = std::stoi(argv[4]);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Invalid delay value. Must be an integer." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Usage: " << argv[0] << " <input.root> <output.root> [-d <delay_ns>]" << std::endl;
            return 1;
        }
    }
    
    measure_lifetime(infile, outfile, delay_ns);
    return 0;
}
