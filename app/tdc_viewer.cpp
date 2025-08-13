#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TStyle.h"

void tdc_viewer(const std::string& filename) {
    TFile* file = TFile::Open(filename.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    TTreeReader reader("tdc_tree", file);
    TTreeReaderValue<UInt_t> channel(reader, "channel");
    TTreeReaderValue<UInt_t> tdc(reader, "tdc");
    TTreeReaderValue<ULong64_t> timestamp(reader, "timestamp");

    gStyle->SetOptStat(1111);

    // 캔버스 생성
    TCanvas* c1 = new TCanvas("c1", "TDC Channel Distributions", 1200, 800);
    c1->Divide(2, 2);

    // 히스토그램 생성
    TH1F* h_hits = new TH1F("h_hits", "Channel Hit Distribution;Channel;Counts", 5, 0.5, 5.5);
    TH1F* h_tdc[4];
    for(int i=0; i<4; ++i) {
        h_tdc[i] = new TH1F(Form("h_tdc_ch%d", i+1), Form("TDC Spectrum CH%d;TDC Value;Counts", i+1), 4096, -0.5, 4095.5);
    }
    
    // 시간 차이 히스토그램 (단위: ps)
    TH1F* h_time_diff = new TH1F("h_time_diff", "Time Difference (CH2 - CH1);Time (ps);Counts", 2000, -10000, 10000);

    std::map<UInt_t, ULong64_t> last_timestamps;
    const ULong64_t time_window = 10000; // ns, Coincidence window

    std::cout << "Processing " << reader.GetEntries() << " events..." << std::endl;
    while (reader.Next()) {
        h_hits->Fill(*channel);
        if (*channel >= 1 && *channel <= 4) {
            h_tdc[*channel - 1]->Fill(*tdc);
        }

        // CH1과 CH2의 시간 차이 계산
        if (*channel == 1) {
            last_timestamps[1] = *timestamp;
        } else if (*channel == 2) {
            if (last_timestamps.count(1) && (*timestamp - last_timestamps[1]) < time_window * 125) { // 125 = 1000ns / 8ns
                // 8ns (timestamp unit) * 1000 -> ps
                double diff_ps = (double)(*timestamp) * 8.0 - (double)(last_timestamps[1]) * 8.0; 
                h_time_diff->Fill(diff_ps);
            }
        }
    }
    std::cout << "Processing complete." << std::endl;

    // 히스토그램 그리기
    c1->cd(1);
    h_hits->Draw();
    
    for(int i=0; i<4; ++i) {
        c1->cd(i + 2);
        h_tdc[i]->Draw();
    }
    
    TCanvas* c2 = new TCanvas("c2", "Timing Resolution", 800, 600);
    c2->cd();
    h_time_diff->Draw();

    std::cout << "Displaying canvases. Close all ROOT windows to exit." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input.root>" << std::endl;
        return 1;
    }
    TApplication app("App", &argc, argv);
    tdc_viewer(argv[1]);
    app.Run();
    return 0;
}
