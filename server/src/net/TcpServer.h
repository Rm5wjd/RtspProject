#pragma once
#include <map>
#include <memory>
#include "RtspSession.h" // 같은 폴더

class TcpServer {
public:
    TcpServer(int port);
    ~TcpServer();
    void start(); 

private:
    int createServerSocket();
    void setNonBlocking(int fd);

    int port;
    int serverFd;
    int epollFd;
    std::map<int, std::unique_ptr<RtspSession>> sessions;
};