#include "RtspSession.h"
#include "RtpSender.h"
#include "utils/base64.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <thread>

RtspSession::RtspSession(int fd, std::string ip, std::shared_ptr<StreamBuffer> streamBuffer) 
    : clientFd(fd), 
      clientIp(ip), 
      streamBuffer_(streamBuffer) 
{
    rtpSender_ = std::make_unique<RtpSender>(streamBuffer_);
    std::cout << "[RTSP] Session created for " << clientIp << std::endl;
}

RtspSession::~RtspSession() {
    if (rtpSender_) {
        rtpSender_->stop();
    }
    close(clientFd);
    std::cout << "[RTSP] Session closed for " << clientIp << std::endl;
}

bool RtspSession::handleEvent() {
    char buffer[4096] = {0};
    int n = recv(clientFd, buffer, sizeof(buffer), 0);
    
    if (n <= 0) return false; 
    
    std::string request(buffer, n);
    // std::cout << "[RTSP] Request:\n" << request << std::endl;
    
    handleRequest(request);
    return true;
}

void RtspSession::sendResponse(const std::string& response) {
    // std::cout << "[RTSP] Response:\n" << response << std::endl;
    send(clientFd, response.c_str(), response.size(), 0);
}

void RtspSession::handleRequest(const std::string& req) {
    std::stringstream ss(req);
    std::string method, url, version;
    ss >> method >> url >> version;

    std::string line, cseq;
    size_t cseq_pos = req.find("CSeq:");
    if (cseq_pos != std::string::npos) {
        size_t end_pos = req.find("\r\n", cseq_pos);
        cseq = req.substr(cseq_pos + 5, end_pos - (cseq_pos + 5));
        cseq.erase(0, cseq.find_first_not_of(" \t\r\n"));
        cseq.erase(cseq.find_last_not_of(" \t\r\n") + 1);
    }

    if (method == "OPTIONS") handleOptions(cseq);
    else if (method == "DESCRIBE") handleDescribe(cseq);
    else if (method == "SETUP") {
        size_t transport_pos = req.find("Transport:");
        std::string transport;
        if (transport_pos != std::string::npos) {
            size_t end_pos = req.find("\r\n", transport_pos);
            transport = req.substr(transport_pos, end_pos - transport_pos);
        }
        handleSetup(cseq, transport);
    }
    else if (method == "PLAY") handlePlay(cseq);
}

void RtspSession::handleOptions(const std::string& cseq) {
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << cseq << "\r\n"
       << "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, OPTIONS\r\n\r\n";
    sendResponse(ss.str());
}

void RtspSession::handleDescribe(const std::string& cseq) {
    std::cout << "[RTSP] Waiting for SPS/PPS from stream..." << std::endl;
    while (!streamBuffer_->hasSpsPps()) {
        if (clientFd < 0) return; // Connection closed
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[RTSP] SPS/PPS are available. Generating SDP." << std::endl;

    auto strip_start_code = [](const std::vector<uint8_t>& nalu) -> std::vector<uint8_t> {
        if (nalu.size() > 4 && nalu[0] == 0 && nalu[1] == 0 && nalu[2] == 0 && nalu[3] == 1) {
            return {nalu.begin() + 4, nalu.end()};
        }
        if (nalu.size() > 3 && nalu[0] == 0 && nalu[1] == 0 && nalu[2] == 1) {
            return {nalu.begin() + 3, nalu.end()};
        }
        return nalu;
    };

    std::vector<uint8_t> sps = streamBuffer_->getSps();
    std::vector<uint8_t> pps = streamBuffer_->getPps();
    
    std::vector<uint8_t> clean_sps = strip_start_code(sps);
    std::vector<uint8_t> clean_pps = strip_start_code(pps);

    std::string sps_b64 = base64_encode(clean_sps);
    std::string pps_b64 = base64_encode(clean_pps);

    std::stringstream sdp;
    sdp << "v=0\r\n"
        << "o=- 12345 67890 IN IP4 " << "0.0.0.0" << "\r\n"
        << "s=Live H.264 Stream\r\n"
        << "c=IN IP4 " << "0.0.0.0" << "\r\n"
        << "t=0 0\r\n"
        << "m=video 0 RTP/AVP 96\r\n"
        << "a=rtpmap:96 H264/90000\r\n"
        << "a=fmtp:96 packetization-mode=1;sprop-parameter-sets=" 
        << sps_b64 << "," << pps_b64 << ";\r\n"
        << "a=control:trackID=0\r\n";

    std::string sdpStr = sdp.str();

    std::stringstream res;
    res << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << cseq << "\r\n"
        << "Content-Type: application/sdp\r\n"
        << "Content-Length: " << sdpStr.size() << "\r\n\r\n"
        << sdpStr;
    
    sendResponse(res.str());
}

void RtspSession::handleSetup(const std::string& cseq, const std::string& transport) {
    if (transport.find("interleaved=") != std::string::npos) {
        std::stringstream res;
        res << "RTSP/1.0 461 Unsupported Transport\r\n"
            << "CSeq: " << cseq << "\r\n\r\n";
        sendResponse(res.str());
        return;
    }

    size_t pos = transport.find("client_port=");
    if (pos != std::string::npos) {
        std::string ports = transport.substr(pos + 12);
        clientRtpPort = std::stoi(ports);
    }

    if (clientRtpPort == 0) {
        std::cerr << "[RTSP-ERROR] Could not parse client_port from: " << transport << std::endl;
        return;
    }

    rtpSender_->init(clientIp, clientRtpPort);

    std::stringstream res;
    res << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << cseq << "\r\n"
        << "Transport: RTP/AVP;unicast;client_port=" << clientRtpPort << "-" << clientRtpPort + 1 
        << ";server_port=30000-30001\r\n" // Example dummy server ports
        << "Session: 12345678\r\n\r\n";
    sendResponse(res.str());
}

void RtspSession::handlePlay(const std::string& cseq) {
    std::stringstream res;
    res << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << cseq << "\r\n"
        << "Range: npt=0.000-\r\n"
        << "Session: 12345678\r\n"
        << "RTP-Info: url=rtsp://0.0.0.0/live/trackID=0\r\n\r\n";
    sendResponse(res.str());

    rtpSender_->start();
}