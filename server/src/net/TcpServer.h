#pragma once
#include <map>
#include <memory>
#include "RtspSession.h"
#include "media/StreamBuffer.h"

class TcpServer {
public:
    TcpServer(int port, std::shared_ptr<StreamBuffer> streamBuffer);
    ~TcpServer();
    void start(); 

private:
    int createServerSocket();
    void setNonBlocking(int fd);

    int port;
    int serverFd;
    int epollFd;
    std::map<int, std::unique_ptr<RtspSession>> sessions;
    std::shared_ptr<StreamBuffer> streamBuffer_;
};