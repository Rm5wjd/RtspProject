#include "TcpServer.h"
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_EVENTS 10

TcpServer::TcpServer(int p, std::shared_ptr<StreamBuffer> streamBuffer) 
    : port(p), streamBuffer_(streamBuffer) {}

TcpServer::~TcpServer() {
    close(serverFd);
    close(epollFd);
}

void TcpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int TcpServer::createServerSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    if (listen(fd, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    return fd;
}

void TcpServer::start() {
    serverFd = createServerSocket();
    epollFd = epoll_create1(0);

    struct epoll_event ev{}, events[MAX_EVENTS];
    ev.events = EPOLLIN; 
    ev.data.fd = serverFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &ev);

    std::cout << "RTSP Server started on port " << port << std::endl;

    while (true) {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == serverFd) { 
                struct sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);
                int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &len);
                
                if (clientFd >= 0) {
                    setNonBlocking(clientFd);
                    
                    ev.events = EPOLLIN;
                    ev.data.fd = clientFd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev);
                    
                    char clientIp[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
                    sessions[clientFd] = std::make_unique<RtspSession>(clientFd, clientIp, streamBuffer_);
                }
            } else { 
                if (sessions.find(fd) != sessions.end()) {
                    bool keepAlive = sessions[fd]->handleEvent();
                    if (!keepAlive) {
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                        sessions.erase(fd);
                    }
                }
            }
        }
    }
}