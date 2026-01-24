#include "net/CameraReceiver.h"
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring> // For strerror

CameraReceiver::CameraReceiver(int port, std::shared_ptr<StreamBuffer> streamBuffer)
    : port_(port), streamBuffer_(streamBuffer) {}

CameraReceiver::~CameraReceiver() {
    stop();
}

void CameraReceiver::start() {
    if (isRunning_) {
        return;
    }
    isRunning_ = true;
    acceptThread_ = std::thread(&CameraReceiver::acceptLoop, this);
    std::cout << "CameraReceiver started on port " << port_ << std::endl;
}

void CameraReceiver::stop() {
    if (!isRunning_) {
        return;
    }
    isRunning_ = false;

    // Shutdown the socket to unblock accept()
    if (serverSocket_ != -1) {
        shutdown(serverSocket_, SHUT_RDWR);
        close(serverSocket_);
        serverSocket_ = -1;
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    std::cout << "CameraReceiver stopped." << std::endl;
}

void CameraReceiver::acceptLoop() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create CameraReceiver socket: " << strerror(errno) << std::endl;
        return;
    }

    // Set SO_REUSEADDR to allow immediate reuse of the port
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
        close(serverSocket_);
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind CameraReceiver socket: " << strerror(errno) << std::endl;
        close(serverSocket_);
        return;
    }

    if (listen(serverSocket_, 1) < 0) {
        std::cerr << "Failed to listen on CameraReceiver socket: " << strerror(errno) << std::endl;
        close(serverSocket_);
        return;
    }

    while (isRunning_) {
        std::cout << "Waiting for camera client connection..." << std::endl;
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            if (isRunning_) {
                 std::cerr << "Accept failed on CameraReceiver socket: " << strerror(errno) << std::endl;
            }
            break; 
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
        std::cout << "Camera client connected from " << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;

        // For this project, we handle one client at a time.
        // To handle multiple clients, you would typically start a new thread here.
        receiveLoop(clientSocket); 

        close(clientSocket);
        std::cout << "Camera client disconnected." << std::endl;
    }

    if(serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

void CameraReceiver::receiveLoop(int clientSocket) {
    int naluCount = 0;
    while (isRunning_) {
        uint32_t naluSize_n; // In network byte order

        // 1. Read the 4-byte NALU size header
        ssize_t bytesRead = recv(clientSocket, &naluSize_n, sizeof(naluSize_n), MSG_WAITALL);
        if (bytesRead <= 0) {
            if (bytesRead < 0) std::cerr << "[RECV] Recv header failed: " << strerror(errno) << std::endl;
            else std::cout << "[RECV] Client closed connection." << std::endl;
            break;
        }

        uint32_t naluSize = ntohl(naluSize_n);
        if (naluSize == 0 || naluSize > 2000000) {
            std::cerr << "[RECV] Invalid NALU size: " << naluSize << std::endl;
            continue;
        }

        // 2. Read the NALU data
        std::vector<uint8_t> nalu(naluSize);
        bytesRead = recv(clientSocket, nalu.data(), naluSize, MSG_WAITALL);
        if (bytesRead <= 0) {
             if (bytesRead < 0) std::cerr << "[RECV] Recv data failed: " << strerror(errno) << std::endl;
             else std::cout << "[RECV] Client closed connection during data read." << std::endl;
            break;
        }

        // --- FIXED Logic: Correctly parse NALU type after start code ---
        if (naluSize > 0) {
            naluCount++;
            
            // Find start code offset within the received chunk
            int offset = 0;
            if (naluSize > 4 && nalu[0] == 0 && nalu[1] == 0 && nalu[2] == 0 && nalu[3] == 1) {
                offset = 4;
            } else if (naluSize > 3 && nalu[0] == 0 && nalu[1] == 0 && nalu[2] == 1) {
                offset = 3;
            }

            if (naluSize > offset) {
                uint8_t naluType = nalu[offset] & 0x1F; // Read type AFTER the start code
                
                if (naluType == 7) { // SPS
                    streamBuffer_->setSps(nalu);
                    std::cout << "[RECV] SPS NALU captured (size: " << nalu.size() << ")" << std::endl;
                } else if (naluType == 8) { // PPS
                    streamBuffer_->setPps(nalu);
                    std::cout << "[RECV] PPS NALU captured (size: " << nalu.size() << ")" << std::endl;
                } else {
                    if (naluCount % 30 == 1) {
                         std::cout << "[RECV] NALU #" << naluCount 
                                   << " (type: " << (int)naluType << ", size: " << nalu.size() << " bytes)" << std::endl;
                    }
                }
            }
        }

        // 3. Push the data to the buffer
        streamBuffer_->push(std::move(nalu));
    }
}
