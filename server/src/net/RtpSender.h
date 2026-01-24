#pragma once
#include "media/StreamBuffer.h"
#include <string>
#include <thread>
#include <atomic>
#include <netinet/in.h>
#include <vector>
#include <memory>

class RtpSender {
public:
    RtpSender(std::shared_ptr<StreamBuffer> streamBuffer);
    ~RtpSender();

    bool init(const std::string& ip, int port);
    void start();
    void stop();

private:
    void sendLoop();
    void sendRtpPacket(const uint8_t* data, int size, uint32_t timestamp, bool mark);

    int sockFd = -1;
    struct sockaddr_in destAddr{};
    std::thread senderThread;
    std::atomic<bool> isRunning{false};
    
    uint16_t seqNum = 0;
    uint32_t timestamp = 0;

    std::shared_ptr<StreamBuffer> streamBuffer_;
};