#include "RtpSender.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <vector>
#include <fstream> // For file dump

#define RTP_MAX_PKT_SIZE 1400

// --- Start of a static file stream for dumping. ---
static std::ofstream g_dumpFile;
static bool g_fileOpened = false;
// --- End of static file stream. ---

struct RtpHeader {
    uint8_t csrcCount : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;
    uint8_t payloadType : 7;
    uint8_t marker : 1;
    uint16_t seq;
    uint32_t timestamp;
    uint32_t ssrc;
};

RtpSender::RtpSender(std::shared_ptr<StreamBuffer> streamBuffer) 
    : streamBuffer_(streamBuffer) 
{
    // Open dump file
    if (!g_fileOpened) {
        g_dumpFile.open("dump.h264", std::ios::binary | std::ios::out | std::ios::trunc);
        if (g_dumpFile.is_open()) {
            std::cout << "[DEBUG] dump.h264 file opened for writing." << std::endl;
            g_fileOpened = true;
        }
    }
}

RtpSender::~RtpSender() {
    stop();
    if (sockFd != -1) close(sockFd);
    if (g_dumpFile.is_open()) {
        g_dumpFile.close();
        g_fileOpened = false;
    }
}

bool RtpSender::init(const std::string& ip, int port) {
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        perror("[RTP] Failed to create socket");
        return false;
    }
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr);
    return true;
}

void RtpSender::start() {
    if (isRunning) return;
    isRunning = true;
    senderThread = std::thread(&RtpSender::sendLoop, this);
    std::cout << "[RTP] Streaming started." << std::endl;
}

void RtpSender::stop() {
    if (!isRunning) return;
    isRunning = false;
    if(streamBuffer_) {
        streamBuffer_->push(std::vector<uint8_t>());
    }
    if (senderThread.joinable()) {
        senderThread.join();
    }
}

void RtpSender::sendLoop() {
    auto strip_start_code = [](const std::vector<uint8_t>& nalu) -> std::vector<uint8_t> {
        if (nalu.size() > 4 && nalu[0] == 0 && nalu[1] == 0 && nalu[2] == 0 && nalu[3] == 1) {
            return {nalu.begin() + 4, nalu.end()};
        }
        if (nalu.size() > 3 && nalu[0] == 0 && nalu[1] == 0 && nalu[2] == 1) {
            return {nalu.begin() + 3, nalu.end()};
        }
        return nalu;
    };

    const char start_code[4] = {0x00, 0x00, 0x00, 0x01};

    while (isRunning) {
        std::vector<uint8_t> nalu_with_sc = streamBuffer_->pop();
        
        if (nalu_with_sc.empty() || !isRunning) {
            break;
        }

        std::vector<uint8_t> clean_nalu = strip_start_code(nalu_with_sc);
        if (clean_nalu.empty()) {
            continue;
        }

        // --- DUMP TO FILE ---
        if (g_dumpFile.is_open()) {
            g_dumpFile.write(start_code, 4);
            g_dumpFile.write((const char*)clean_nalu.data(), clean_nalu.size());
        }
        // --- END DUMP ---

        int naluSize = clean_nalu.size();
        uint8_t* naluData = clean_nalu.data();
        uint8_t naluHeader = naluData[0];
        uint8_t naluType = naluHeader & 0x1F;
        bool isVcl = (naluType >= 1 && naluType <= 5);
        bool marker = isVcl; 

        if (naluSize <= RTP_MAX_PKT_SIZE) {
            sendRtpPacket(naluData, naluSize, timestamp, marker);
        } else {
            const uint8_t* payload = naluData + 1;
            int payloadSize = naluSize - 1;
            int offset = 0;
            while (offset < payloadSize) {
                int len = RTP_MAX_PKT_SIZE - 2;
                bool isLastFragment = (offset + len >= payloadSize);
                if (isLastFragment) {
                    len = payloadSize - offset;
                }
                uint8_t fragBuf[1500];
                fragBuf[0] = (naluHeader & 0xE0) | 28;
                fragBuf[1] = naluType;
                if (offset == 0) fragBuf[1] |= 0x80;
                else if (isLastFragment) fragBuf[1] |= 0x40;
                memcpy(fragBuf + 2, payload + offset, len);
                bool finalPacketMarker = isLastFragment && marker;
                sendRtpPacket(fragBuf, len + 2, timestamp, finalPacketMarker);
                offset += len;
            }
        }

        if (isVcl) {
            timestamp += 90000 / 30;
        }
    }
    std::cout << "[RTP] sendLoop stopped." << std::endl;
}

void RtpSender::sendRtpPacket(const uint8_t* data, int size, uint32_t ts, bool mark) {
    if (sockFd < 0) return;
    RtpHeader header;
    header.version = 2;
    header.padding = 0;
    header.extension = 0;
    header.csrcCount = 0;
    header.marker = mark ? 1 : 0;
    header.payloadType = 96;
    header.seq = htons(seqNum++);
    header.timestamp = htonl(ts);
    header.ssrc = htonl(0x12345678);
    uint8_t buffer[1500];
    memcpy(buffer, &header, sizeof(RtpHeader));
    memcpy(buffer + sizeof(RtpHeader), data, size);
    int totalLen = sizeof(RtpHeader) + size;
    sendto(sockFd, buffer, totalLen, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
}