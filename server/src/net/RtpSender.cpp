#include "RtpSender.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>

#define RTP_MAX_PKT_SIZE 1400

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

RtpSender::RtpSender() {}

RtpSender::~RtpSender() {
    stop();
    if (fp) fclose(fp);
    if (sockFd != -1) close(sockFd);
}

bool RtpSender::init(const std::string& ip, int port) {
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) return false;

    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr);

    // 파일 열기 (프로젝트 루트에 있는 test.h264)
    // 1. 상위 폴더(프로젝트 루트)에서 먼저 찾아본다. (빌드 폴더에서 실행 시)
	fp = fopen("../test.h264", "rb");

	// 2. 만약 없으면 현재 폴더에서도 찾아본다. (혹시 실행 파일을 밖으로 꺼냈을 경우 대비)
	if (!fp) {
		fp = fopen("test.h264", "rb");
	}
    if (!fp) {
        perror("[RTP] test.h264 file not found");
        return false;
    }
    return true;
}

void RtpSender::start() {
    if (isRunning) return;
    isRunning = true;
    senderThread = std::thread(&RtpSender::sendLoop, this);
    std::cout << "[RTP] Streaming started using test.h264" << std::endl;
}

void RtpSender::stop() {
    isRunning = false;
    if (senderThread.joinable()) senderThread.join();
}

// ---------------------------------------------------------
// [핵심] Start Code(00 00 00 01)를 기준으로 NALU 분리
// ---------------------------------------------------------
int RtpSender::readNextNalu(uint8_t* buffer, int maxSize) {
    if (!fp) return -1;

    int pos = 0;
    int state = 0; // 0:초기, 1:0발견, 2:00발견, 3:000발견
    uint8_t byte;
    bool foundStartCode = false;

    while (fread(&byte, 1, 1, fp) > 0) {
        if (pos >= maxSize) break; // 버퍼 보호
        buffer[pos++] = byte;

        // Start Code 패턴 감지 (00 00 00 01 또는 00 00 01)
        if (state == 0 && byte == 0x00) state = 1;
        else if (state == 1 && byte == 0x00) state = 2;
        else if (state == 2 && byte == 0x00) state = 3;
        else if ((state == 2 || state == 3) && byte == 0x01) {
            
            // Start Code 발견!
            // 하지만 이게 "파일 맨 처음"에 있는 Start Code라면? -> 무시하고 계속 읽어야 함
            // 이게 "데이터 뒤"에 나온 Start Code라면? -> 여기가 이번 NAL의 끝임
            
            // Start Code 길이 (3바이트 or 4바이트)
            int startCodeLen = (state == 3) ? 4 : 3;

            // [핵심 수정] 
            // 현재 읽은 위치(pos)가 Start Code 길이와 같다면? 
            // -> 즉, 방금 읽기 시작했는데 바로 Start Code가 나온 경우 (파일 시작 or 이전 NAL 끝난 직후)
            // -> 이건 "이번 NAL의 시작점"이므로 멈추지 말고 계속 데이터를 읽어야 함!
            if (pos <= 4) {
                // Start Code를 버퍼에 포함시키고 계속 진행
                // state만 초기화
                state = 0;
                continue; 
            }

            // 여기까지 왔다면 "다음 NAL의 시작"을 만난 것임.
            // 파일 포인터를 Start Code 길이만큼 뒤로 감아서, 다음 호출 때 읽게 해줌
            fseek(fp, -startCodeLen, SEEK_CUR);

            // 현재 버퍼에서 뒤에 붙은 Start Code는 제거하고 리턴
            return pos - startCodeLen;
        } 
        else {
            // 00 00 00 01 패턴이 깨짐 -> 일반 데이터임
            state = 0;
        }
    }

    // 파일 끝(EOF)에 도달한 경우
    // 마지막 NAL Unit은 뒤에 Start Code가 없으므로 여기까지 읽은 걸 그대로 리턴
    return pos;
}

void RtpSender::sendLoop() {
    // NALU 담을 넉넉한 버퍼
    std::vector<uint8_t> naluBuf(1024 * 500); // 500KB

    while (isRunning) {
        int naluSize = readNextNalu(naluBuf.data(), naluBuf.size());
		//std::cout << "[DEBUG] Read NALU Size: " << naluSize << std::endl;
        if (naluSize <= 0) {
			//std::cout << "[DEBUG] Nalu Size is 0 or less. Rewinding..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        uint8_t* naluData = naluBuf.data();
        
        // H.264 NAL Header (1 byte)
        // F(1) | NRI(2) | Type(5)
        uint8_t naluHeader = naluData[0]; 
        int type = naluHeader & 0x1F;

        // ---------------------------------------------------
        // Case 1: 작은 패킷 (Single NAL Unit) -> 그냥 보냄
        // ---------------------------------------------------
        if (naluSize <= RTP_MAX_PKT_SIZE) {
            sendRtpPacket(naluData, naluSize, timestamp, true);
        } 
        // ---------------------------------------------------
        // Case 2: 큰 패킷 (Fragmentation Unit, FU-A) -> 쪼개서 보냄
        // ---------------------------------------------------
        else {
            // NAL Header 생략하고 Payload부터 시작
            const uint8_t* payload = naluData + 1; 
            int payloadSize = naluSize - 1;
            int offset = 0;

            while (offset < payloadSize) {
                int len = RTP_MAX_PKT_SIZE - 2; // FU Header(2byte) 공간 확보
                bool isLast = false;

                if (offset + len >= payloadSize) {
                    len = payloadSize - offset;
                    isLast = true;
                }

                // [FU Indicator] (1 byte)
                // F | NRI | Type(28 for FU-A)
                uint8_t fuIndicator = (naluHeader & 0xE0) | 28;

                // [FU Header] (1 byte)
                // S | E | R | Type
                uint8_t fuHeader = (naluHeader & 0x1F);
                if (offset == 0) fuHeader |= 0x80; // Start bit
                else if (isLast) fuHeader |= 0x40; // End bit

                uint8_t fragBuf[1500];
                fragBuf[0] = fuIndicator;
                fragBuf[1] = fuHeader;
                memcpy(fragBuf + 2, payload + offset, len);

                sendRtpPacket(fragBuf, len + 2, timestamp, isLast);
                offset += len;
            }
        }

        // 프레임 레이트 조절 (30fps)
        timestamp += 90000 / 30;
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

void RtpSender::sendRtpPacket(const uint8_t* data, int size, uint32_t ts, bool mark) {
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

    // UDP 버퍼 조립
    // [RTP Header] + [Data (NALU or FU-A)]
    uint8_t buffer[1500];
    memcpy(buffer, &header, sizeof(RtpHeader));
    memcpy(buffer + sizeof(RtpHeader), data, size);

	
    int totalLen = sizeof(RtpHeader) + size;
	int sentBytes = sendto(sockFd, buffer, totalLen, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if (sentBytes < 0) {
        // 전송 실패 시 에러 메시지 출력 (권한 문제, 소켓 문제 등)
        perror("[Error] UDP sendto failed"); 
    } else {
        // 너무 많이 뜨면 정신없으니 주석 처리하거나, 가끔 확인용으로 켜세요.
        //std::cout << "[DEBUG] Sent " << sentBytes << " bytes to Port " << ntohs(destAddr.sin_port) << std::endl;
    }
    sendto(sockFd, buffer, totalLen, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
}