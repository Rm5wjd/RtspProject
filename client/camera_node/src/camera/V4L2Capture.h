#pragma once
#include <string>
#include <vector>

struct Buffer {
    void* start;
    size_t length;
};

class V4L2Capture {
public:
    V4L2Capture(const std::string& device = "/dev/video0");
    ~V4L2Capture();

    // 해상도와 포맷 설정 (기본 640x480, YUV420)
    // *주의: Raw YUV는 용량이 커서 일단 작은 해상도로 테스트
    bool init(int width = 640, int height = 480);
    
    // 캡처 시작/정지
    bool startCapture();
    void stopCapture();

    // 프레임 한 장 가져오기 (타임아웃 적용)
    // 데이터는 내부 버퍼 포인터로 반환 (복사 비용 절약)
    bool grabFrame(void** outData, size_t* outSize);
    
    // 가져온 버퍼 반납 (필수)
    void releaseFrame();

private:
    std::string deviceName;
    int fd = -1;
    std::vector<Buffer> buffers;
    int currentBufferIndex = -1;
};