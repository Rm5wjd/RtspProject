#include "camera/V4L2Capture.h"
#include "network/TcpClient.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <cstring>

// Helper function to find the next start code
const uint8_t* findStartCode(const uint8_t* p, const uint8_t* end) {
    while (p < end - 3) {
        if (p[0] == 0 && p[1] == 0) {
            if (p[2] == 1) return p;
            if (p[2] == 0 && p + 4 <= end && p[3] == 1) return p;
        }
        p++;
    }
    return end;
}

// Helper function to parse a buffer and send NAL units one by one
void parseAndSendNalus(const uint8_t* data, size_t size, TcpClient& client) {
    if (size == 0) return;

    const uint8_t* buffer_end = data + size;
    const uint8_t* nalu_start = findStartCode(data, buffer_end);

    while (nalu_start < buffer_end) {
        int startCodeLen = (nalu_start[2] == 1) ? 3 : 4;
        const uint8_t* nalu_data_start = nalu_start + startCodeLen;
        const uint8_t* next_nalu_start = findStartCode(nalu_data_start, buffer_end);
        size_t nalu_size = next_nalu_start - nalu_start;

        if (nalu_size > 0) {
            client.sendData((void*)nalu_start, nalu_size);
        }
        
        nalu_start = next_nalu_start;
    }
}


int main() {
    // 1. 설정
    std::string serverIp = "192.168.219.105"; // ★ PC(서버) IP로 변경 필수!
    int serverPort = 8556;                // 서버 수신 포트

    // 2. 객체 생성
    V4L2Capture camera("/dev/video0");
    TcpClient client;

    // 3. 카메라 초기화 (1920x1080)
    if (!camera.init(1920, 1080)) {
        std::cerr << "Camera init failed" << std::endl;
        return -1;
    }

    // 4. 서버 연결
    while (!client.connectToServer(serverIp, serverPort)) {
        std::cout << "Waiting for server..." << std::endl;
        sleep(2);
    }
    std::cout << "Server connected." << std::endl;

    // 5. 캡처 시작
    if (!camera.startCapture()) {
        std::cerr << "Failed to start camera capture" << std::endl;
        return -1;
    }
    std::cout << "Capture started. Streaming..." << std::endl;
    
    // 6. 메인 루프: 프레임을 잡아 NAL 유닛 단위로 파싱 후 전송
    while (true) {
        void* frameData = nullptr;
        size_t frameSize = 0;
        
        if (camera.grabFrame(&frameData, &frameSize)) {
            if (frameSize > 0) {
                parseAndSendNalus((const uint8_t*)frameData, frameSize, client);
            }
            camera.releaseFrame();
        } else {
            std::cerr << "Frame grab failed!" << std::endl;
            usleep(100000); // 0.1초 대기
        }
        
        // 프레임레이트와 유사한 딜레이 (카메라 드라이버가 보통 맞춰줌)
        usleep(10000); 
    }

    return 0;
}