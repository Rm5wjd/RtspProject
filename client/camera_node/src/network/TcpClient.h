#pragma once
#include <string>
#include <vector>
#include <cstdint>

class TcpClient {
public:
    TcpClient();
    ~TcpClient();

    bool connectToServer(const std::string& ip, int port);
    void disconnect();
    
    // [Length Header(4B)] + [Data Payload] 전송
    bool sendData(const void* data, size_t size);

private:
    int sockFd = -1;
};