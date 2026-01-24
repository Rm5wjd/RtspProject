#include "net/TcpServer.h"
#include "media/StreamBuffer.h"
#include "net/CameraReceiver.h"
#include <memory>
#include <thread>
#include <csignal>
#include <iostream>

// For signal handler to access servers
std::unique_ptr<CameraReceiver> g_pReceiver;
// TcpServer is blocking on the main thread, so we can't stop it from here.
// std::unique_ptr<TcpServer> g_pRtspServer;

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    
    if (g_pReceiver) {
        std::cout << "Stopping Camera Receiver..." << std::endl;
        g_pReceiver->stop();
    }
    
    // Since TcpServer blocks the main thread, we exit here.
    std::cout << "Exiting application." << std::endl;
    exit(signum);
}


int main() {
    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    // 1. Create the shared buffer
    auto streamBuffer = std::make_shared<StreamBuffer>();
    std::cout << "Main: StreamBuffer created." << std::endl;

    // 2. Start the camera data receiver in a background thread
    g_pReceiver = std::make_unique<CameraReceiver>(8556, streamBuffer);
    g_pReceiver->start();

    // 3. Start the RTSP server (this will block the main thread)
    TcpServer rtspServer(8554, streamBuffer);
    rtspServer.start(); 

    // --- The following code is unreachable because rtspServer.start() blocks ---
    std::cout << "Shutting down..." << std::endl;
    g_pReceiver->stop();

    return 0;
}