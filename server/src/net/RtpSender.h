#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <netinet/in.h>
#include <vector>

class RtpSender {
public:
    RtpSender();
    ~RtpSender();

    bool init(const std::string& ip, int port);
    void start();
    void stop();

private:
    void sendLoop();
    void sendRtpPacket(const uint8_t* data, int size, uint32_t timestamp, bool mark);
    
    // NAL Unit 하나를 읽어오는 헬퍼 함수
    int readNextNalu(uint8_t* buffer, int maxSize);

    int sockFd = -1;
    struct sockaddr_in destAddr{};
    std::thread senderThread;
    std::atomic<bool> isRunning{false};
    
    uint16_t seqNum = 0;
    uint32_t timestamp = 0;
    
    // H.264 파일 관련
    FILE* fp = nullptr;
};