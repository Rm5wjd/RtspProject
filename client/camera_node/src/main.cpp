#include "camera/V4L2Capture.h"
#include "network/TcpClient.h"
#include <iostream>
#include <unistd.h>
#include <chrono>

int main() {
    // 1. 설정
    std::string serverIp = "192.168.219.101"; // ★ PC(서버) IP로 변경 필수!
    int serverPort = 8554;                // 서버 수신 포트

    // 2. 객체 생성
    V4L2Capture camera("/dev/video0");
    TcpClient client;

    // 3. 카메라 초기화 (640x480)
    if (!camera.init(1920, 1080)) {
        std::cerr << "Camera init failed" << std::endl;
        return -1;
    }

    // 4. 서버 연결
    while (!client.connectToServer(serverIp, serverPort)) {
        std::cout << "Waiting for server..." << std::endl;
        sleep(2);
    }

    // 5. 캡처 및 전송 루프
    if (!camera.startCapture()) return -1;

    std::cout << "Start streaming..." << std::endl;
    
    void* frameData = nullptr;
    size_t frameSize = 0;
    int frameCount = 0;

    while (true) {
        // [Capture]
        if (camera.grabFrame(&frameData, &frameSize)) {
            
            // [Send]
            if (!client.sendData(frameData, frameSize)) {
                std::cerr << "Send failed. Connection lost?" << std::endl;
                break;
            }

            // [Release]
            camera.releaseFrame();

            // 로그 (30프레임마다)
            if (++frameCount % 30 == 0) {
                std::cout << "Sent 30 frames. (Last size: " << frameSize << " bytes)" << std::endl;
            }
        } else {
            std::cerr << "Frame drop!" << std::endl;
        }
        
        // 너무 빠르면 네트워크 막히니까 살짝 딜레이 (나중에 인코더 넣으면 삭제)
        usleep(10000); 
    }

    return 0;
}