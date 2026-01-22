#include "TcpClient.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>   // IPPROTO_TCP를 위해 필요
#include <netinet/tcp.h>  // TCP_NODELAY를 위해 필요

TcpClient::TcpClient() {}
TcpClient::~TcpClient() { disconnect(); }

bool TcpClient::connectToServer(const std::string& ip, int port) {
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) return false;

    // ★ [추가] TCP_NODELAY 설정 (Nagle 알고리즘 비활성화)
    // 데이터가 작아도 모으지 않고 즉시 전송하게 만듦
    int flag = 1;
    if (setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) < 0) {
        perror("Setsockopt TCP_NODELAY failed");
        // 실패해도 연결은 계속 진행하거나, 여기서 return false 할지 결정 (보통은 진행)
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &servAddr.sin_addr) <= 0) return false;

    if (connect(sockFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("Connection Failed");
        return false;
    }
    std::cout << "[Network] Connected to " << ip << ":" << port << std::endl;
    return true;
}

void TcpClient::disconnect() {
    if (sockFd != -1) {
        close(sockFd);
        sockFd = -1;
    }
}

bool TcpClient::sendData(const void* data, size_t size) {
    if (sockFd == -1) return false;

    // 1. 길이 헤더 전송 (Network Byte Order)
    uint32_t netLen = htonl(static_cast<uint32_t>(size));
    int sent = send(sockFd, &netLen, 4, 0);
    if (sent != 4) return false;

    // 2. 실제 데이터 전송
    // 한 번에 다 못 보낼 수도 있으므로 루프 처리
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    size_t remaining = size;
    
    while (remaining > 0) {
        int n = send(sockFd, ptr, remaining, 0);
        if (n < 0) return false;
        ptr += n;
        remaining -= n;
    }
    return true;
}