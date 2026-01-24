#pragma once

#include "media/StreamBuffer.h"
#include <memory>
#include <thread>
#include <atomic>

class CameraReceiver {
public:
    CameraReceiver(int port, std::shared_ptr<StreamBuffer> streamBuffer);
    ~CameraReceiver();

    void start();
    void stop();

private:
    void acceptLoop();
    void receiveLoop(int clientSocket);

    int port_;
    std::shared_ptr<StreamBuffer> streamBuffer_;
    int serverSocket_ = -1;
    
    std::atomic<bool> isRunning_{false};
    std::thread acceptThread_;
};
