#include "net/TcpServer.h" // 경로 수정됨

int main() {
    // 8554 포트로 서버 시작
    TcpServer server(8554); 
    server.start();
    return 0;
}