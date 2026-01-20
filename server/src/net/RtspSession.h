#pragma once
#include "RtpSender.h" // 같은 폴더에 있으므로 바로 include
#include <string>

class RtspSession {
public:
    RtspSession(int fd, std::string clientIp);
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
    RtpSender rtpSender;
    int clientRtpPort = 0;
};