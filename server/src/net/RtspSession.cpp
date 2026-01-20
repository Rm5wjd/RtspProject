#include "RtspSession.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

RtspSession::RtspSession(int fd, std::string ip) : clientFd(fd), clientIp(ip) {}

RtspSession::~RtspSession() {
    close(clientFd);
    std::cout << "[RTSP] Session closed: " << clientIp << std::endl;
}

bool RtspSession::handleEvent() {
    char buffer[4096] = {0};
    int n = recv(clientFd, buffer, sizeof(buffer), 0);
    
    if (n <= 0) return false; 
    
    std::string request(buffer, n);
    std::cout << "[RTSP] Request:\n" << request << std::endl;
    
    handleRequest(request);
    return true;
}

void RtspSession::sendResponse(const std::string& response) {
    send(clientFd, response.c_str(), response.size(), 0);
    std::cout << "[RTSP] Response sent." << std::endl;
}

void RtspSession::handleRequest(const std::string& req) {
    std::stringstream ss(req);
    std::string method, url, version;
    ss >> method >> url >> version;

    std::string line, cseq;
    while (std::getline(ss, line) && line != "\r") {
        if (line.find("CSeq:") != std::string::npos) {
            cseq = line.substr(line.find(":") + 1);
            cseq.erase(0, cseq.find_first_not_of(" \t\r\n"));
            cseq.erase(cseq.find_last_not_of(" \t\r\n") + 1);
        }
    }

    if (method == "OPTIONS") handleOptions(cseq);
    else if (method == "DESCRIBE") handleDescribe(cseq);
    else if (method == "SETUP") {
        std::stringstream ss2(req);
        std::string transport;
        while (std::getline(ss2, line) && line != "\r") {
            if (line.find("Transport:") != std::string::npos) {
                transport = line;
                break;
            }
        }
        handleSetup(cseq, transport);
    }
    else if (method == "PLAY") handlePlay(cseq);
}

void RtspSession::handleOptions(const std::string& cseq) {
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << cseq << "\r\n"
       << "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY\r\n\r\n";
    sendResponse(ss.str());
}

void RtspSession::handleDescribe(const std::string& cseq) {
    std::stringstream sdp;
    sdp << "v=0\r\n"
        << "o=- 999 999 IN IP4 0.0.0.0\r\n"
        << "s=MyRtspServer\r\n"
        << "c=IN IP4 0.0.0.0\r\n"
        << "t=0 0\r\n"
        << "m=video 0 RTP/AVP 96\r\n"
        << "a=rtpmap:96 H264/90000\r\n"
        << "a=fmtp:96 packetization-mode=1;profile-level-id=42001f;sprop-parameter-sets=Z0IAH5WoFAI=,aM48gA==;\r\n"
        << "a=control:track0\r\n";

    std::string sdpStr = sdp.str();

    std::stringstream res;
    res << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << cseq << "\r\n"
        << "Content-Base: rtsp://0.0.0.0:8554/live/\r\n"
        << "Content-Type: application/sdp\r\n"
        << "Content-Length: " << sdpStr.size() << "\r\n\r\n"
        << sdpStr;
    
    sendResponse(res.str());
}

void RtspSession::handleSetup(const std::string& cseq, const std::string& transport) {
    // 1. TCP 요청(Interleaved)이 오면 "지원 안 함" 에러 보내기
    if (transport.find("interleaved=") != std::string::npos) {
        std::stringstream res;
        res << "RTSP/1.0 461 Unsupported Transport\r\n"
            << "CSeq: " << cseq << "\r\n\r\n";
        sendResponse(res.str());
        std::cout << "[RTSP] Rejected TCP request (UDP only supported)" << std::endl;
        return;
    }

    // 2. UDP 포트 파싱 (기존 로직)
    size_t pos = transport.find("client_port=");
    if (pos != std::string::npos) {
        std::string ports = transport.substr(pos + 12);
        size_t dash = ports.find('-');
        if (dash != std::string::npos) {
            clientRtpPort = std::stoi(ports.substr(0, dash));
        }
    }

    // 포트를 못 찾았으면 에러 처리
    if (clientRtpPort == 0) {
        std::cerr << "[Error] Could not parse client_port from: " << transport << std::endl;
        return;
    }

    rtpSender.init(clientIp, clientRtpPort);

    // ... (응답 전송 로직은 그대로) ...
    std::stringstream res;
    res << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << cseq << "\r\n"
        << "Transport: RTP/AVP;unicast;client_port=" << clientRtpPort << "-" << clientRtpPort+1 
        << ";server_port=8000-8001\r\n"
        << "Session: 12345678\r\n\r\n";
    sendResponse(res.str());
}

void RtspSession::handlePlay(const std::string& cseq) {
    std::stringstream res;
    res << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << cseq << "\r\n"
        << "Range: npt=0.000-\r\n"
        << "Session: 12345678\r\n\r\n";
    sendResponse(res.str());

    rtpSender.start();
}