#pragma once
#include "media/StreamBuffer.h"
#include <string>
#include <memory>

class RtpSender; // Forward declaration

class RtspSession {
public:
    RtspSession(int fd, std::string clientIp, std::shared_ptr<StreamBuffer> streamBuffer);
    ~RtspSession();

    bool handleEvent(); 

private:
    void handleRequest(const std::string& request);
    void sendResponse(const std::string& response);
    
    void handleOptions(const std::string& cseq);
    void handleDescribe(const std::string& cseq);
    void handleSetup(const std::string& cseq, const std::string& transport);
    void handlePlay(const std::string& cseq);

    int clientFd;
    std::string clientIp;
    
    std::unique_ptr<RtpSender> rtpSender_;
    std::shared_ptr<StreamBuffer> streamBuffer_;

    int clientRtpPort = 0;
};