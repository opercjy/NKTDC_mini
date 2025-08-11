#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NoticeTDC4CH.h"

// TDC 한 채널을 교정하는 함수
int calib_TDC(int tcp_Handle, int ch);

// 메인 함수를 자동화 로직으로 수정
int main(int argc, char *argv[]) {
    int tcp_Handle;
    char ip[100];
    int ch;

    // IP 주소는 터미널 인자 혹은 기본값으로 설정
    if (argc > 1) {
        sprintf(ip, "%s", argv[1]);
    } else {
        sprintf(ip, "192.168.0.2");
    }

    // --- 자동화 시퀀스 시작 ---

    // 1. TDC 열기
    printf("Connecting to TDC at %s...\n", ip);
    tcp_Handle = TDC4CHopen(ip);
    if (tcp_Handle < 0) {
        printf("Failed to connect to TDC. Exiting.\n");
        return -1;
    }
    TDC4CHinit_TDC(tcp_Handle);

    // 2. 모든 채널(1-4) 순차적으로 교정
    for (ch = 1; ch <= 4; ch++) {
        printf("\n----- Starting Calibration for Channel %d -----\n", ch);
        if (calib_TDC(tcp_Handle, ch) != 0) {
            fprintf(stderr, "Error during calibration of channel %d.\n", ch);
            TDC4CHclose(tcp_Handle);
            return -1;
        }
        printf("----- Calibration for Channel %d Finished -----\n", ch);
    }

    // 3. 생성된 채널별 .lut 파일을 하나의 tdc_cal.lut 파일로 병합
    printf("\nMerging calibration files into tdc_cal.lut...\n");
    char *data = (char *)malloc(8192);
    char wfilename[256];
    char rfilename[256];
    FILE *wfp;
    FILE *rfp;

    sprintf(wfilename, "tdc_cal.lut");
    wfp = fopen(wfilename, "wb");
    if (!wfp) {
        fprintf(stderr, "Error: Cannot create final LUT file %s.\n", wfilename);
        free(data);
        TDC4CHclose(tcp_Handle);
        return -1;
    }

    for (ch = 0; ch < 4; ch++) {
        sprintf(rfilename, "tdc_cal_%d.lut", ch + 1);
        rfp = fopen(rfilename, "rb");
        if (!rfp) {
            fprintf(stderr, "Error: Cannot open intermediate LUT file %s.\n", rfilename);
            free(data);
            fclose(wfp);
            TDC4CHclose(tcp_Handle);
            return -1;
        }
        fread(data, 1, 8192, rfp);
        fclose(rfp);
        fwrite(data, 1, 8192, wfp);
    }
    fclose(wfp);
    free(data);
    printf("Successfully created tdc_cal.lut.\n");

    // 4. TDC 닫기
    TDC4CHclose(tcp_Handle);
    printf("\nCalibration process complete.\n");

    return 0;
}

// TDC 한 채널을 교정하는 상세 로직
int calib_TDC(int tcp_Handle, int ch) {
    char filename[256];
    FILE *lfp;
    int i;
    int hist[4096];
    char *data;
    int count;
    int data_size;
    int tdc;
    long long target_events = 40960000;
    short cal_val[4096];
    char lut_val[8192];

    // 교정을 위해 해당 채널의 문턱값만 낮추고 나머지는 최대로 설정
    TDC4CHwrite_THR(tcp_Handle, 1, 255);
    TDC4CHwrite_THR(tcp_Handle, 2, 255);
    TDC4CHwrite_THR(tcp_Handle, 3, 255);
    TDC4CHwrite_THR(tcp_Handle, 4, 255);
    TDC4CHwrite_THR(tcp_Handle, ch, 10);

    TDC4CHwrite_RAW_MODE(tcp_Handle, 1); // 원시 데이터 모드 설정
    TDC4CHreset(tcp_Handle);
    sprintf(filename, "tdc_cal_%d.lut", ch);
    lfp = fopen(filename, "wb");
    if (!lfp) {
        fprintf(stderr, "Error: Cannot create LUT file for channel %d\n", ch);
        return -1;
    }

    for (i = 0; i < 4096; i++) hist[i] = 0;
    
    TDC4CHwrite_ACQ_TIME(tcp_Handle, 0); // 무한정 획득
    data = (char *)malloc(4096);
    TDC4CHstart(tcp_Handle);

    count = 0;
    while (count < target_events) {
        data_size = TDC4CHread_DATA_SIZE(tcp_Handle);
        if ((count + data_size) > target_events) {
            data_size = target_events - count;
        }
        if (data_size) {
            TDC4CHread_DATA(tcp_Handle, data_size, data);
            for (i = 0; i < data_size; i++) {
                tdc = (unsigned char)data[i * 8] | ((unsigned char)data[i * 8 + 1] << 8);
                hist[tdc]++;
            }
            count += data_size;
            printf("Channel %d: %d / %lld events taken\r", ch, count, target_events);
            fflush(stdout);
        }
    }
    printf("\n");

    TDC4CHstop(tcp_Handle);
    free(data);
    TDC4CHreset(tcp_Handle);
    TDC4CHwrite_RAW_MODE(tcp_Handle, 0);

    // 교정 테이블 계산
    double cnt_all = count; // 실제 수집된 이벤트 수로 계산
    double bin_begin = 0.0, bin_end = 0.0, cnt_begin = 0.0, cnt_end = 0.0;
    for (i = 0; i < 4096; i++) {
        cnt_end += hist[4095 - i];
        bin_end += ((cnt_end - cnt_begin) / cnt_all * 1000.0);
        cal_val[4095 - i] = (int)((bin_end + bin_begin) / 2.0 + 0.5);
        cnt_begin = cnt_end;
        bin_begin = bin_end;
    }
    cal_val[0] = 0;
    for (i = 1; i < 4096; i++) cal_val[i] += 1;

    memcpy(lut_val, cal_val, 8192);
    fwrite(lut_val, 1, 8192, lfp);
    fclose(lfp);
    return 0;
}
